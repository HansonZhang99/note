# linux内核lcd基本框架

linux内核版本：linux4.9.88

## 内核lcd框架

分为上下两层：

* fbmem.c：承上启下
  * 实现、注册file_operations结构体
  * 把APP的调用向下转发到具体的硬件驱动程序
* xxx_fb.c：硬件相关的驱动程序
  * 实现、注册fb_info结构体
  * 实现硬件操作

### 通用框架fbmem

drivers/video/fbdev/core/fbmem.c

```cpp
static int __init
fbmem_init(void)
{
	int ret;

	if (!proc_create("fb", 0, NULL, &fb_proc_fops))
		return -ENOMEM;

	ret = register_chrdev(FB_MAJOR, "fb", &fb_fops);//注册主设备号29,绑定fb_fops
	if (ret) {
		printk("unable to get major %d for fb devs\n", FB_MAJOR);
		goto err_chrdev;
	}

	fb_class = class_create(THIS_MODULE, "graphics");//创建类
	if (IS_ERR(fb_class)) {
		ret = PTR_ERR(fb_class);
		pr_warn("Unable to create fb class; errno = %d\n", ret);
		fb_class = NULL;
		goto err_class;
	}
	return 0;

err_class:
	unregister_chrdev(FB_MAJOR, "fb");
err_chrdev:
	remove_proc_entry("fb", NULL);
	return ret;
}
```

fbmem_init函数为lcd子系统申请了主设备号29，注册了lcd的class名为graphics。这里没有创建实际的设备，通常字符设备的创建会调用device_add/device_create在dev目录创建设备节点，在sys目录创建对应的调试目录。

对于字符设备，操作/dev/下的lcd设备时，一定会先调用fb_fops中的对用函数，如open

```cpp
static int
fb_open(struct inode *inode, struct file *file)
__acquires(&info->lock)
__releases(&info->lock)
{
	int fbidx = iminor(inode);
	struct fb_info *info;
	int res = 0;

	info = get_fb_info(fbidx);//通过次设备号找到对应的lcd描述符，fb_info用来描述一个lcd设备，linux内核将各种lcd设备都统一成这个结构体，与硬件相关
	if (!info) {
		request_module("fb%d", fbidx);
		info = get_fb_info(fbidx);
		if (!info)
			return -ENODEV;
	}
	if (IS_ERR(info))
		return PTR_ERR(info);

	mutex_lock(&info->lock);
	if (!try_module_get(info->fbops->owner)) {
		res = -ENODEV;
		goto out;
	}
	file->private_data = info;//
	if (info->fbops->fb_open) {
		res = info->fbops->fb_open(info,1);//调用fb_info的open函数
		if (res)
			module_put(info->fbops->owner);
	}
#ifdef CONFIG_FB_DEFERRED_IO
	if (info->fbdefio)
		fb_deferred_io_open(info, inode, file);
#endif
out:
	mutex_unlock(&info->lock);
	if (res)
		put_fb_info(info);
	return res;
}
```

write函数：

```cpp
static ssize_t
fb_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	struct fb_info *info = file_fb_info(file);//获取open时保存的info结构
	u8 *buffer, *src;
	u8 __iomem *dst;
	int c, cnt = 0, err = 0;
	unsigned long total_size;

	if (!info || !info->screen_base)
		return -ENODEV;

	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;

	if (info->fbops->fb_write)
		return info->fbops->fb_write(info, buf, count, ppos);//调用info的write,直接返回
	//当info中没有write函数时，执行以下代码
	total_size = info->screen_size;//lcd显存长度(已经被iormap)

	if (total_size == 0)
		total_size = info->fix.smem_len;//显存长度

	if (p > total_size)
		return -EFBIG;

	if (count > total_size) {
		err = -EFBIG;
		count = total_size;
	}

	if (count + p > total_size) {
		if (!err)
			err = -ENOSPC;

		count = total_size - p;
	}
	//上面全是长度判断
	buffer = kmalloc((count > PAGE_SIZE) ? PAGE_SIZE : count,
			 GFP_KERNEL);//分配内存
	if (!buffer)
		return -ENOMEM;

	dst = (u8 __iomem *) (info->screen_base + p);//获取写入起始地址

	if (info->fbops->fb_sync)
		info->fbops->fb_sync(info);

	while (count) {
		c = (count > PAGE_SIZE) ? PAGE_SIZE : count;
		src = buffer;

		if (copy_from_user(src, buf, c)) {//从用户空间拷贝数据到src
			err = -EFAULT;
			break;
		}

		fb_memcpy_tofb(dst, src, c);//将src的数据给到显存
		dst += c;
		src += c;
		*ppos += c;
		buf += c;
		cnt += c;
		count -= c;
	}

	kfree(buffer);

	return (cnt) ? cnt : err;
}
```

从上面的open/write可以看出，最终都会调用fb_info的对应函数。fb_info结构体是内核对lcd硬件的抽象，所有的lcd都需要实现这个结构，这些都是驱动工程师的工作，如下是2440的lcd注册

### 硬件抽象s3c2410fb

drivers/video/fbdev/s3c2410fb.c

```cpp
static int s3c2412fb_probe(struct platform_device *pdev)
{
	return s3c24xxfb_probe(pdev, DRV_S3C2410);
}

static int s3c24xxfb_probe(struct platform_device *pdev,
			   enum s3c_drv_type drv_type)
{
	struct s3c2410fb_info *info;
	struct s3c2410fb_display *display;
	struct fb_info *fbinfo;
	struct s3c2410fb_mach_info *mach_info;
	struct resource *res;
	int ret;
	int irq;
	int i;
	int size;
	u32 lcdcon1;

	mach_info = dev_get_platdata(&pdev->dev);
	if (mach_info == NULL) {
		dev_err(&pdev->dev,
			"no platform data for lcd, cannot attach\n");
		return -EINVAL;
	}

	if (mach_info->default_display >= mach_info->num_displays) {
		dev_err(&pdev->dev, "default is %d but only %d displays\n",
			mach_info->default_display, mach_info->num_displays);
		return -EINVAL;
	}

	display = mach_info->displays + mach_info->default_display;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no irq for device\n");
		return -ENOENT;
	}

	fbinfo = framebuffer_alloc(sizeof(struct s3c2410fb_info), &pdev->dev);//分配一个fb_info + s3c2410fb_info结构，会按照long对齐最后长度
	if (!fbinfo)
		return -ENOMEM;

	platform_set_drvdata(pdev, fbinfo);//pdev->dev->driver_data = fbinfo

	info = fbinfo->par;
	info->dev = &pdev->dev;
	info->drv_type = drv_type;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);//获取设备需要的资源
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to get memory registers\n");
		ret = -ENXIO;
		goto dealloc_fb;
	}

	size = resource_size(res);
	info->mem = request_mem_region(res->start, size, pdev->name);//请求分配资源
	if (info->mem == NULL) {
		dev_err(&pdev->dev, "failed to get memory region\n");
		ret = -ENOENT;
		goto dealloc_fb;
	}

	info->io = ioremap(res->start, size);//物理地址虚拟地址映射
	if (info->io == NULL) {
		dev_err(&pdev->dev, "ioremap() of registers failed\n");
		ret = -ENXIO;
		goto release_mem;
	}

	if (drv_type == DRV_S3C2412)
		info->irq_base = info->io + S3C2412_LCDINTBASE;
	else
		info->irq_base = info->io + S3C2410_LCDINTBASE;

	dprintk("devinit\n");

	strcpy(fbinfo->fix.id, driver_name);

	/* Stop the video */
	//硬件相关的设置
	lcdcon1 = readl(info->io + S3C2410_LCDCON1);
	writel(lcdcon1 & ~S3C2410_LCDCON1_ENVID, info->io + S3C2410_LCDCON1);

	fbinfo->fix.type	    = FB_TYPE_PACKED_PIXELS;
	fbinfo->fix.type_aux	    = 0;
	fbinfo->fix.xpanstep	    = 0;
	fbinfo->fix.ypanstep	    = 0;
	fbinfo->fix.ywrapstep	    = 0;
	fbinfo->fix.accel	    = FB_ACCEL_NONE;

	fbinfo->var.nonstd	    = 0;
	fbinfo->var.activate	    = FB_ACTIVATE_NOW;
	fbinfo->var.accel_flags     = 0;
	fbinfo->var.vmode	    = FB_VMODE_NONINTERLACED;
	//硬件相关的file_operations，lcd子系统中的设备的操作最终会导致此结构体的函数被调用
	fbinfo->fbops		    = &s3c2410fb_ops;
	fbinfo->flags		    = FBINFO_FLAG_DEFAULT;
	fbinfo->pseudo_palette      = &info->pseudo_pal;

	for (i = 0; i < 256; i++)
		info->palette_buffer[i] = PALETTE_BUFF_CLEAR;
	//中断
	ret = request_irq(irq, s3c2410fb_irq, 0, pdev->name, info);
	if (ret) {
		dev_err(&pdev->dev, "cannot get irq %d - err %d\n", irq, ret);
		ret = -EBUSY;
		goto release_regs;
	}
	//时钟
	info->clk = clk_get(NULL, "lcd");
	if (IS_ERR(info->clk)) {
		dev_err(&pdev->dev, "failed to get lcd clock source\n");
		ret = PTR_ERR(info->clk);
		goto release_irq;
	}

	clk_prepare_enable(info->clk);
	dprintk("got and enabled clock\n");

	usleep_range(1000, 1100);

	info->clk_rate = clk_get_rate(info->clk);
	//计算显存
	/* find maximum required memory size for display */
	for (i = 0; i < mach_info->num_displays; i++) {
		unsigned long smem_len = mach_info->displays[i].xres;

		smem_len *= mach_info->displays[i].yres;
		smem_len *= mach_info->displays[i].bpp;
		smem_len >>= 3;
		if (fbinfo->fix.smem_len < smem_len)
			fbinfo->fix.smem_len = smem_len;
	}

	/* Initialize video memory */
	//分配显存，dma
	ret = s3c2410fb_map_video_memory(fbinfo);
	if (ret) {
		dev_err(&pdev->dev, "Failed to allocate video RAM: %d\n", ret);
		ret = -ENOMEM;
		goto release_clock;
	}

	dprintk("got video memory\n");

	fbinfo->var.xres = display->xres;
	fbinfo->var.yres = display->yres;
	fbinfo->var.bits_per_pixel = display->bpp;
	//初始化lcd寄存器
	s3c2410fb_init_registers(fbinfo);

	s3c2410fb_check_var(&fbinfo->var, fbinfo);

	ret = s3c2410fb_cpufreq_register(info);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to register cpufreq\n");
		goto free_video_memory;
	}
	//注册lcd设备，这个函数会调用device_create
	ret = register_framebuffer(fbinfo);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to register framebuffer device: %d\n",
			ret);
		goto free_cpufreq;
	}

	/* create device files */
	//属性文件
	ret = device_create_file(&pdev->dev, &dev_attr_debug);
	if (ret)
		dev_err(&pdev->dev, "failed to add debug attribute\n");

	dev_info(&pdev->dev, "fb%d: %s frame buffer device\n",
		fbinfo->node, fbinfo->fix.id);

	return 0;

 free_cpufreq:
	s3c2410fb_cpufreq_deregister(info);
free_video_memory:
	s3c2410fb_unmap_video_memory(fbinfo);
release_clock:
	clk_disable_unprepare(info->clk);
	clk_put(info->clk);
release_irq:
	free_irq(irq, info);
release_regs:
	iounmap(info->io);
release_mem:
	release_mem_region(res->start, size);
dealloc_fb:
	framebuffer_release(fbinfo);
	return ret;
}
```

probe函数中完成设备注册，主要流程：

* 分配fb_info

  * framebuffer_alloc

    

* 设置fb_info

  * var
  * fbops
  * 硬件相关操作

* 注册fb_info

  * register_framebuffer



## 自己编写lcd驱动框架(基于QEMU)

```cpp
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/io.h>

#include <asm/div64.h>

#include <asm/mach/map.h>



struct lcd_regs
{
	volatile unsigned int fb_base_phys;
	volatile unsigned int fb_xres;
	volatile unsigned int fb_yres;
	volatile unsigned int fb_bpp;
	
};

struct fb_info *fbinfo = NULL;

static unsigned int pseudo_palette[16];

static struct lcd_regs *g_lcd_regs;

static inline u_int chan_to_field(u_int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

static int mylcd_setcolreg(unsigned regno,
			       unsigned red, unsigned green, unsigned blue,
			       unsigned transp, struct fb_info *info)
{
	unsigned int val;

	/* dprintk("setcol: regno=%d, rgb=%d,%d,%d\n",
		   regno, red, green, blue); */

	switch (info->fix.visual) {
	case FB_VISUAL_TRUECOLOR:
		/* true-colour, use pseudo-palette */

		if (regno < 16) {
			u32 *pal = info->pseudo_palette;

			val  = chan_to_field(red,   &info->var.red);
			val |= chan_to_field(green, &info->var.green);
			val |= chan_to_field(blue,  &info->var.blue);

			pal[regno] = val;
		}
		break;

	default:
		return 1;	/* unknown type */
	}

	return 0;
}
				   

static struct fb_ops myfb_ops = {
	.owner		= THIS_MODULE,
	.fb_setcolreg	= mylcd_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

int __init lcd_drv_init(void)
{
	dma_addr_t dma_addr;
	//分配fb_info
	fbinfo = framebuffer_alloc(0, NULL);
	//设置fb_info
	//var
	fbinfo->var.xres = 1024;
	fbinfo->var.yres = 600;
	fbinfo->var.bits_per_pixel = 16;

	fbinfo->var.red.offset = 11;
	fbinfo->var.red.length = 5;

	fbinfo->var.green.offset = 5;
	fbinfo->var.green.length = 6;

	fbinfo->var.blue.offset = 0;
	fbinfo->var.blue.length = 5;
	//fix

	strcpy(fbinfo->fix.id, "100ask_lcd");
	fbinfo->fix.smem_len = fbinfo->var.xres * fbinfo->var.yres * fbinfo->var.bits_per_pixel / 8;
	if( fbinfo->var.bits_per_pixel == 24)
	{
		fbinfo->fix.smem_len = fbinfo->var.xres * fbinfo->var.yres * 4;
	}
	
	//虚拟地址
	fbinfo->screen_base = dma_alloc_wc(NULL, fbinfo->fix.smem_len, &dma_addr, GFP_KERNEL);

	//物理地址
	fbinfo->fix.smem_start = dma_addr;
	fbinfo->fix.type = FB_TYPE_PACKED_PIXELS;
	fbinfo->fix.visual = FB_VISUAL_TRUECOLOR;

	fbinfo->fix.line_length = fbinfo->var.xres * fbinfo->var.bits_per_pixel / 8;
	if (fbinfo->var.bits_per_pixel == 24)
		fbinfo->fix.line_length = fbinfo->var.xres * 4;
	
	fbinfo->fbops = &myfb_ops;
	fbinfo->pseudo_palette = pseudo_palette;
	//注册fb_info
	register_framebuffer(fbinfo);

	//硬件操作
	g_lcd_regs = ioremap(0x021c8000, sizeof(struct lcd_regs));
	g_lcd_regs->fb_base_phys = dma_addr;
	g_lcd_regs->fb_xres = 500;
	g_lcd_regs->fb_yres = 300;
	g_lcd_regs->fb_bpp = 16;
	return 0;
}

static void __exit lcd_drv_cleanup(void)
{
	unregister_framebuffer(fbinfo);
	framebuffer_release(fbinfo);
	iounmap(g_lcd_regs);
}

module_init(lcd_drv_init);
module_exit(lcd_drv_cleanup);

MODULE_AUTHOR("Hanson Zhang");
MODULE_LICENSE("GPL");

```

## APP(基于QEMU)

编译不通过，仅仅用来学习

```cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <linux/fb.h>

#include "common.h"

static struct fb_info fb_info;
//清楚区域的颜色
static void fb_clear_area(struct fb_info *fb_info, int x, int y, int w, int h)
{
	int i = 0;
	int loc;
	char *fbuffer = (char *)fb_info->ptr;
	struct fb_var_screeninfo *var = &fb_info->var;
	struct fb_fix_screeninfo *fix = &fb_info->fix;

	for (i = 0; i < h; i++) {
		loc = (x + var->xoffset) * (var->bits_per_pixel / 8)
			+ (y + i + var->yoffset) * fix->line_length;
		memset(fbuffer + loc, 0, w * var->bits_per_pixel / 8);
	}
}
//向lcd放入一个字符，字符有坐标和颜色，字符从字库中获取，分辨率是8*8
static void fb_put_char(struct fb_info *fb_info, int x, int y, char c,
		unsigned color)
{
	int i, j, bits, loc;
	unsigned char *p8;
	unsigned short *p16;
	unsigned int *p32;
	struct fb_var_screeninfo *var = &fb_info->var;
	struct fb_fix_screeninfo *fix = &fb_info->fix;

	for (i = 0; i < 8; i++) {
		bits = fontdata_8x8[8 * c + i];
		for (j = 0; j < 8; j++) {
			loc = (x + j + var->xoffset) * (var->bits_per_pixel / 8)
				+ (y + i + var->yoffset) * fix->line_length;
			if (loc >= 0 && loc < fix->smem_len &&
					((bits >> (7 - j)) & 1)) {
				switch (var->bits_per_pixel) {
				case 8:
					p8 =  fb_info->ptr + loc;
					*p8 = color;
				case 16:
					p16 = fb_info->ptr + loc;
					*p16 = color;
					break;
				case 24:
				case 32:
					p32 = fb_info->ptr + loc;
					*p32 = color;
					break;
				}
			}
		}
	}
}
//同上，字符串
int fb_put_string(struct fb_info *fb_info, int x, int y, char *s, int maxlen,
		int color, int clear, int clearlen)
{
	int i;
	int w = 0;

	if (clear)
		fb_clear_area(fb_info, x, y, clearlen * 8, 8);

	for (i = 0; i < strlen(s) && i < maxlen; i++) {
		fb_put_char(fb_info, (x + 8 * i), y, s[i], color);
		w += 8;
	}

	return w;
}
void fb_open(int fb_num, struct fb_info *fb_info)
{
	char str[64];
	int fd,tty;

	tty = open("/dev/tty1", O_RDWR);

	if(ioctl(tty, KDSETMODE, KD_GRAPHICS) == -1)
		printf("Failed to set graphics mode on tty1\n");

	sprintf(str, "/dev/fb%d", fb_num);
	fd = open(str, O_RDWR);//打开具体的lcd设备

	ASSERT(fd >= 0);

	fb_info->fd = fd;//保存描述符
	IOCTL1(fd, FBIOGET_VSCREENINFO, &fb_info->var);//lcd可变信息
	IOCTL1(fd, FBIOGET_FSCREENINFO, &fb_info->fix);//lcd固定信息

	printf("fb res %dx%d virtual %dx%d, line_len %d, bpp %d\n",
			fb_info->var.xres, fb_info->var.yres,
			fb_info->var.xres_virtual, fb_info->var.yres_virtual,
			fb_info->fix.line_length, fb_info->var.bits_per_pixel);

	void *ptr = mmap(0,
			fb_info->var.yres_virtual * fb_info->fix.line_length,
			PROT_WRITE | PROT_READ,
			MAP_SHARED, fd, 0);//调用mmap获取framebuffer显存，内核会调用mmap,将framebuffer物理内存映射到这里

	ASSERT(ptr != MAP_FAILED);

	fb_info->ptr = ptr;
}

//填充颜色，向lcd的x,y坐标填充颜色，这里会考虑8bit  16bit 24bit的像素
static void draw_pixel(struct fb_info *fb_info, int x, int y, unsigned color)
{
	void *fbmem;

	fbmem = fb_info->ptr;
	if (fb_info->var.bits_per_pixel == 8) {
		unsigned char *p;

		fbmem += fb_info->fix.line_length * y;

		p = fbmem;

		p += x;

		*p = color;
	} else if (fb_info->var.bits_per_pixel == 16) {
		unsigned short c;
		unsigned r = (color >> 16) & 0xff;
		unsigned g = (color >> 8) & 0xff;
		unsigned b = (color >> 0) & 0xff;
		unsigned short *p;

		r = r * 32 / 256;
		g = g * 64 / 256;
		b = b * 32 / 256;

		c = (r << 11) | (g << 5) | (b << 0);

		fbmem += fb_info->fix.line_length * y;

		p = fbmem;

		p += x;

		*p = c;
	} else if (fb_info->var.bits_per_pixel == 24) {
		 unsigned char *p;

		p = (unsigned char *)fbmem + fb_info->fix.line_length * y + 3 * x;
		*p++ = color;
		*p++ = color >> 8;
		*p = color >> 16;
	} else {
		unsigned int *p;

		fbmem += fb_info->fix.line_length * y;

		p = fbmem;

		p += x;

		*p = color;
	}
}

//填满整个lcd屏幕
static void fill_screen(struct fb_info *fb_info)
{
	unsigned x, y;
	unsigned h = fb_info->var.yres;
	unsigned w = fb_info->var.xres;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			if (x < 20 && y < 20)
				draw_pixel(fb_info, x, y, 0xffffff);
			else if (x < 20 && (y > 20 && y < h - 20))
				draw_pixel(fb_info, x, y, 0xff);
			else if (y < 20 && (x > 20 && x < w - 20))
				draw_pixel(fb_info, x, y, 0xff00);
			else if (x > w - 20 && (y > 20 && y < h - 20))
				draw_pixel(fb_info, x, y, 0xff0000);
			else if (y > h - 20 && (x > 20 && x < w - 20))
				draw_pixel(fb_info, x, y, 0xffff00);
			else if (x == 20 || x == w - 20 ||
					y == 20 || y == h - 20)
				draw_pixel(fb_info, x, y, 0xffffff);
			else if (x == y || w - x == h - y)
				draw_pixel(fb_info, x, y, 0xff00ff);
			else if (w - x == y || x == h - y)
				draw_pixel(fb_info, x, y, 0x00ffff);
			else if (x > 20 && y > 20 && x < w - 20 && y < h - 20) {
				int t = x * 3 / w;
				unsigned r = 0, g = 0, b = 0;
				unsigned c;
				if (fb_info->var.bits_per_pixel == 16) {
					if (t == 0)
						b = (y % 32) * 256 / 32;
					else if (t == 1)
						g = (y % 64) * 256 / 64;
					else if (t == 2)
						r = (y % 32) * 256 / 32;
				} else {
					if (t == 0)
						b = (y % 256);
					else if (t == 1)
						g = (y % 256);
					else if (t == 2)
						r = (y % 256);
				}
				c = (r << 16) | (g << 8) | (b << 0);
				draw_pixel(fb_info, x, y, c);
			} else {
				draw_pixel(fb_info, x, y, 0);
			}
		}

	}
	//向lcd写字符串
	fb_put_string(fb_info, w / 3 * 2, 30, "RED", 3, 0xffffff, 1, 3);
	fb_put_string(fb_info, w / 3, 30, "GREEN", 5, 0xffffff, 1, 5);
	fb_put_string(fb_info, 20, 30, "BLUE", 4, 0xffffff, 1, 4);
}

//纯色填充整个屏幕
void fill_screen_solid(struct fb_info *fb_info, unsigned int color)
{

	unsigned x, y;
	unsigned h = fb_info->var.yres;
	unsigned w = fb_info->var.xres;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++)
			draw_pixel(fb_info, x, y, color);
	}
}


static void do_fill_screen(struct fb_info *fb_info, int pattern)
{

	switch (pattern) {
	case 1:
		fill_screen_solid(fb_info, 0xff0000);
		break;
	case 2:
		fill_screen_solid(fb_info, 0x00ff00);
		break;
	case 3:
		fill_screen_solid(fb_info, 0x0000ff);
		break;
	case 4:
		fill_screen_solid(fb_info, 0xffffff);
		break;
	case 0:
	default:
		fill_screen(fb_info);
		break;
	}
}

void show_help(void)
{
	printf("Usage: fb-test -f fbnum -r -g -b -w -p pattern\n");
	printf("Where -f fbnum   = framebuffer device number\n");
	printf("      -r         = fill framebuffer with red\n");
	printf("      -g         = fill framebuffer with green\n");
	printf("      -b         = fill framebuffer with blue\n");
	printf("      -w         = fill framebuffer with white\n");
	printf("      -p pattern = fill framebuffer with pattern number\n");
}

int main(int argc, char **argv)
{
	int opt;
	int req_fb = 0;
	int req_pattern = 0;

	printf("fb-test %d.%d.%d (%s)\n", VERSION, PATCHLEVEL, SUBLEVEL,
		VERSION_NAME);

	while ((opt = getopt(argc, argv, "hrgbwp:f:")) != -1) {
		switch (opt) {
		case 'f':
			req_fb = atoi(optarg);
			break;
		case 'p':
			req_pattern = atoi(optarg);
			break;
		case 'r':
			req_pattern = 1;
			break;
		case 'g':
			req_pattern = 2;
			break;
		case 'b':
			req_pattern = 3;
			break;
		case 'w':
			req_pattern = 4;
			break;
		case 'h':
			show_help();
			return 0;
		default:
			exit(EXIT_FAILURE);
		}
	}

	fb_open(req_fb, &fb_info);//打开设备

	do_fill_screen(&fb_info, req_pattern);//填充颜色

	return 0;
}

```

