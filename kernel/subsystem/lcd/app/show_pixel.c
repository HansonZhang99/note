#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>

static int fd_fb;
static struct fb_var_screeninfo var;	/* Current var */
static struct fb_fix_screeninfo fix;	/* Current var */

static int screen_size;
static unsigned char *fb_base;
static unsigned int line_width;
static unsigned int pixel_width;

/**********************************************************************
 * 函数名称： lcd_put_pixel
 * 功能描述： 在LCD指定位置上输出指定颜色（描点）
 * 输入参数： x坐标，y坐标，颜色
 * 输出参数： 无
 * 返 回 值： 会
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2020/05/12	     V1.0	  zh(angenao)	      创建
 ***********************************************************************/ 
void lcd_put_pixel(void *fb_base, int x, int y, unsigned int color)
{
	unsigned char *pen_8 = fb_base+y*line_width+x*pixel_width;
	unsigned short *pen_16;	
	unsigned int *pen_32;	

	unsigned int red, green, blue;	

	pen_16 = (unsigned short *)pen_8;
	pen_32 = (unsigned int *)pen_8;

	switch (var.bits_per_pixel)
	{
		case 8:
		{
			*pen_8 = color;
			break;
		}
		case 16:
		{
			/* 565 */
			red   = (color >> 16) & 0xff;
			green = (color >> 8) & 0xff;
			blue  = (color >> 0) & 0xff;
			color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
			*pen_16 = color;
			break;
		}
		case 32:
		{
			*pen_32 = color;
			break;
		}
		default:
		{
			printf("can't surport %dbpp\n", var.bits_per_pixel);
			break;
		}
	}
}

void lcd_draw_screen(void *addr, unsigned int color)
{
	int i, j;
	for(i = 0; i < var.xres; i++)
		for(j = 0; j < var.yres; j++)
			lcd_put_pixel(addr, x, y, color);
}
int main(int argc, char **argv)
{
	int i;
	int nBuffers;
	int nNextBuffer;
	void *nextBufferAddr = NULL;
	int ret;
	unsigned int colors[] = {0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000, 0x00ffffff};//0x00rrggbb
	if(argc != 2)
	{
		printf("./a.out n[ n is an integer which is used to represent buffer count\n");
		return 0;
	}
	fd_fb = open("/dev/fb0", O_RDWR);
	if (fd_fb < 0)
	{
		printf("can't open /dev/fb0\n");
		return -1;
	}
	if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &var))
	{
		printf("can't get var\n");
		return -1;
	}
	if (ioctl(fd_fb, FBIOGET_FSCREENINFO, &fix))
	{
		printf("can't get fix\n");
		return -1;
	}
	
	line_width  = var.xres * var.bits_per_pixel / 8;
	pixel_width = var.bits_per_pixel / 8;
	screen_size = var.xres * var.yres * var.bits_per_pixel / 8;
	printf("var.bits_per_pixel = %d\n", var.bits_per_pixel);
	nBuffers = fix.smem_len / screen_size;
	printf("kernel alloc %d buffers\n", nBuffers);
	nBuffers = nBuffers > atoi(argv[1]) ? atoi(argv[1]) : nBuffers;
	printf("app use %d buffers\n", nBuffers);
	fb_base = (unsigned char *)mmap(NULL , fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
	if (fb_base == (unsigned char *)-1)
	{
		printf("can't mmap\n");
		return -1;
	}

	if(atoi(argv[1]) == 1 || nBuffers == 1)
	{
		while(1)
		{
			for(i = 0; i < sizeof(colors) / sizeof(colors[i]); i++)
			{
				lcd_draw_screen(fb_base, color[i]);
				usleep(1000 * 1000);
			}
			
		}
	}
	else 
	{
		var.yres_virtual = nBuffers * var.yres;
		ioctl(fd_fb, FBIOPUT_VSCREENINFO, &var);
		nNextBuffer = 0;
		nextBufferAddr = fb_base;
		while(1)
		{
			for(i = 0; i < sizeof(colors) / sizeof(colors[i]); i++)
			{
				lcd_draw_screen(nextBufferAddr, color[i]);
				var.yoffset = nNextBuffer * var.yres;
				ioctl(fd_fb, FBIOPAN_DISPLAY, &var);
				ret = 0;
				ioctl(fd_fb, FBIO_WAITFORVSYNC, &ret);
				nNextBuffer ++;
				if(nNextBuffer == nBuffers)
					nNextBuffer = 0;
				nextBufferAddr =  fb_base + nNextBuffer * screen_size;
				usleep(1000 * 1000);
			}
			
		}
	}

	
	
	munmap(fb_base , screen_size);
	close(fd_fb);
	
	return 0;	
}

