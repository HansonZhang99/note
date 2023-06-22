# imx6ull lcd驱动程序分析

## 如何找到imx6ull的lcd驱动程序相关文件
拿到厂商的内核，直接编译后在drivers/video/fbdev下找到被编译的文件

```shell
zhhhhhh@ubuntu:~/bsp/IMX6ULL/100ask_imx6ull-sdk/Linux-4.9.88/drivers/video/fbdev$ ls *.o
built-in.o  mx3fb.o  mxsfb.o
```

或者make menuconfig，在配置选项中找到被勾选的选项。

从上面可以看出使用的应该是mxsfb.c文件

## 程序分析

### 入口platform_driver

```cpp
static struct platform_driver mxsfb_driver = {
	.probe = mxsfb_probe,
	.remove = mxsfb_remove,
	.shutdown = mxsfb_shutdown,
	.id_table = mxsfb_devtype,
	.driver = {
		   .name = DRIVER_NAME,
		   .of_match_table = mxsfb_dt_ids,
		   .pm = &mxsfb_pm_ops,
	},
};

module_platform_driver(mxsfb_driver);
```

向内核注册了platform_driver，通过of_match_table中的名字，在dts目录中寻找设备

```cpp
static const struct of_device_id mxsfb_dt_ids[] = {
	{ .compatible = "fsl,imx23-lcdif", .data = &mxsfb_devtype[0], },
	{ .compatible = "fsl,imx28-lcdif", .data = &mxsfb_devtype[1], },
	{ .compatible = "fsl,imx7ulp-lcdif", .data = &mxsfb_devtype[2], },
	{ /* sentinel */ }
};
```

### platform device来自dts

最终可以找到"fsl,imx28-lcdif"属性对应的dts文件和位置：

```shell
zhhhhhh@ubuntu:~/bsp/IMX6ULL/100ask_imx6ull-sdk/Linux-4.9.88/arch/arm/boot/dts$ grep "fsl,imx28-lcdif" -rn ./*.dtsi
./imx28.dtsi:994:                               compatible = "fsl,imx28-lcdif";
./imx6sl.dtsi:834:                              compatible = "fsl,imx6sl-lcdif", "fsl,imx28-lcdif";
./imx6sll.dtsi:670:                             compatible = "fsl,imx6sll-lcdif", "fsl,imx28-lcdif";
./imx6sx.dtsi:1382:                                     compatible = "fsl,imx6sx-lcdif", "fsl,imx28-lcdif";
./imx6sx.dtsi:1394:                                     compatible = "fsl,imx6sx-lcdif", "fsl,imx28-lcdif";
./imx6ul.dtsi:1101:                             compatible = "fsl,imx6ul-lcdif", "fsl,imx28-lcdif";
./imx6ull.dtsi:1017:                            compatible = "fsl,imx6ul-lcdif", "fsl,imx28-lcdif";
./imx7s.dtsi:681:                               compatible = "fsl,imx7d-lcdif", "fsl,imx28-lcdif";
```

如下是imx6ull soc的lcd dts部分:

```shell
lcdif: lcdif@021c8000 {
        compatible = "fsl,imx6ul-lcdif", "fsl,imx28-lcdif";
        reg = <0x021c8000 0x4000>;
        interrupts = <GIC_SPI 5 IRQ_TYPE_LEVEL_HIGH>;
        clocks = <&clks IMX6UL_CLK_LCDIF_PIX>,
                 <&clks IMX6UL_CLK_LCDIF_APB>,
                 <&clks IMX6UL_CLK_DUMMY>;
        clock-names = "pix", "axi", "disp_axi";
        status = "disabled";
    };
```

单板lcd相关的dts部分如下：

arch/arm/boot/dts/100ask_imx6ull-14x14.dts

```shell
&lcdif {
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_lcdif_dat
             &pinctrl_lcdif_ctrl
             &pinctrl_lcdif_reset>;
    display = <&display0>;
    status = "okay";
    reset-gpios = <&gpio3 4 GPIO_ACTIVE_LOW>; /* 100ask */

    display0: display {
        bits-per-pixel = <24>;
        bus-width = <24>;

        display-timings {
            native-mode = <&timing0>;

             timing0: timing0_1024x768 {
             clock-frequency = <50000000>;
             hactive = <1024>;
             vactive = <600>;
             hfront-porch = <160>;
             hback-porch = <140>;
             hsync-len = <20>;
             vback-porch = <20>;
             vfront-porch = <12>;
             vsync-len = <3>;

             hsync-active = <0>;
             vsync-active = <0>;
             de-active = <1>;
             pixelclk-active = <0>;
             };

        };
    };
};

```

dts会被转换为platform_device，并与platform_driver匹配，导致probe函数被调用：

### platform匹配后导致probe调用

```cpp
static int mxsfb_probe(struct platform_device *pdev)
{
	const struct of_device_id *of_id =
			of_match_device(mxsfb_dt_ids, &pdev->dev);
	struct resource *res;
	struct mxsfb_info *host;
	struct fb_info *fb_info;
	struct pinctrl *pinctrl;
	int irq = platform_get_irq(pdev, 0);
	int gpio, ret;
	int rst_gpio;

	if (of_id)
		pdev->id_entry = of_id->data;

	gpio = of_get_named_gpio(pdev->dev.of_node, "enable-gpio", 0);
	if (gpio == -EPROBE_DEFER)
		return -EPROBE_DEFER;

	if (gpio_is_valid(gpio)) {
		ret = devm_gpio_request_one(&pdev->dev, gpio, GPIOF_OUT_INIT_LOW, "lcd_pwr_en");
		if (ret) {
			dev_err(&pdev->dev, "faild to request gpio %d, ret = %d\n", gpio, ret);
			return ret;
		}
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);//获取硬件需要的内存io资源
	if (!res) {
		dev_err(&pdev->dev, "Cannot get memory IO resource\n");
		return -ENODEV;
	}

	host = devm_kzalloc(&pdev->dev, sizeof(struct mxsfb_info), GFP_KERNEL);//host是硬件自定义结构，里面包含fb_info,分配一个host结构
	if (!host) {
		dev_err(&pdev->dev, "Failed to allocate IO resource\n");
		return -ENOMEM;
	}

	fb_info = framebuffer_alloc(0, &pdev->dev);//分配一个fb_info结构
	if (!fb_info) {
		dev_err(&pdev->dev, "Failed to allocate fbdev\n");
		devm_kfree(&pdev->dev, host);
		return -ENOMEM;
	}
	host->fb_info = fb_info;
	fb_info->par = host;

	ret = devm_request_irq(&pdev->dev, irq, mxsfb_irq_handler, 0,
			  dev_name(&pdev->dev), host);//申请中断
	if (ret) {
		dev_err(&pdev->dev, "request_irq (%d) failed with error %d\n",
				irq, ret);
		ret = -ENODEV;
		goto fb_release;
	}

	host->base = devm_ioremap_resource(&pdev->dev, res);//将lcd控制寄存器硬件物理地址映射成虚拟
	if (IS_ERR(host->base)) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = PTR_ERR(host->base);
		goto fb_release;
	}

	host->pdev = pdev;
	platform_set_drvdata(pdev, host);

	host->devdata = &mxsfb_devdata[pdev->id_entry->driver_data];

	host->clk_pix = devm_clk_get(&host->pdev->dev, "pix");//时钟设置
	if (IS_ERR(host->clk_pix)) {
		host->clk_pix = NULL;
		ret = PTR_ERR(host->clk_pix);
		goto fb_release;
	}

	host->clk_axi = devm_clk_get(&host->pdev->dev, "axi");//时钟设置
	if (IS_ERR(host->clk_axi)) {
		host->clk_axi = NULL;
		ret = PTR_ERR(host->clk_axi);
		dev_err(&pdev->dev, "Failed to get axi clock: %d\n", ret);
		goto fb_release;
	}

	host->clk_disp_axi = devm_clk_get(&host->pdev->dev, "disp_axi");//时钟设置
	if (IS_ERR(host->clk_disp_axi)) {
		host->clk_disp_axi = NULL;
		ret = PTR_ERR(host->clk_disp_axi);
		dev_err(&pdev->dev, "Failed to get disp_axi clock: %d\n", ret);
		goto fb_release;
	}

	host->reg_lcd = devm_regulator_get(&pdev->dev, "lcd");
	if (IS_ERR(host->reg_lcd))
		host->reg_lcd = NULL;

	fb_info->pseudo_palette = devm_kzalloc(&pdev->dev, sizeof(u32) * 16,
					       GFP_KERNEL);
	if (!fb_info->pseudo_palette) {
		ret = -ENOMEM;
		dev_err(&pdev->dev, "Failed to allocate pseudo_palette memory\n");
		goto fb_release;
	}

	INIT_LIST_HEAD(&fb_info->modelist);

	pm_runtime_enable(&host->pdev->dev);

	ret = mxsfb_init_fbinfo(host);//初始化fb_info
	if (ret != 0) {
		dev_err(&pdev->dev, "Failed to initialize fbinfo: %d\n", ret);
		goto fb_pm_runtime_disable;
	}

	ret = mxsfb_dispdrv_init(pdev, fb_info);
	if (ret != 0) {
		if (ret == -EPROBE_DEFER)
			dev_info(&pdev->dev,
				 "Defer fb probe due to dispdrv not ready\n");
		goto fb_free_videomem;
	}

	if (!host->dispdrv) {
		pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
		if (IS_ERR(pinctrl)) {
			ret = PTR_ERR(pinctrl);
			goto fb_pm_runtime_disable;
		}
	}

	if (!host->enabled) {
		writel(0, host->base + LCDC_CTRL);
		mxsfb_set_par(fb_info);//设置寄存器
		mxsfb_enable_controller(fb_info);//激活lcd
		pm_runtime_get_sync(&host->pdev->dev);
	}

	ret = register_framebuffer(fb_info);//向内核注册lcd设备
	if (ret != 0) {
		dev_err(&pdev->dev, "Failed to register framebuffer\n");
		goto fb_destroy;
	}

	mxsfb_overlay_init(host);

#ifndef CONFIG_FB_IMX64_DEBUG
	console_lock();
	ret = fb_blank(fb_info, FB_BLANK_UNBLANK);
	console_unlock();
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to unblank framebuffer\n");
		goto fb_unregister;
	}
#endif
        /* 100ask */
        printk("100ask, %s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
        rst_gpio = of_get_named_gpio(pdev->dev.of_node, "reset-gpios", 0);
        if (gpio_is_valid(rst_gpio)) {
                ret = gpio_request(rst_gpio, "lcdif_rst");
                if (ret < 0) {
                        dev_err(&pdev->dev,
                                "Failed to request GPIO:%d, ERRNO:%d\n",
                                (s32)rst_gpio, ret);
                } else {
                        gpio_direction_output(rst_gpio, 0);
                        msleep(2);
                        gpio_direction_output(rst_gpio, 1);
                        dev_info(&pdev->dev,  "Success seset LCDIF\n");
                }
        }

	dev_info(&pdev->dev, "initialized\n");

	return 0;

#ifndef CONFIG_FB_IMX64_DEBUG
fb_unregister:
	unregister_framebuffer(fb_info);
#endif
fb_destroy:
	fb_destroy_modelist(&fb_info->modelist);
fb_free_videomem:
	mxsfb_free_videomem(host);
fb_pm_runtime_disable:
	clk_disable_pix(host);
	clk_disable_axi(host);
	clk_disable_disp_axi(host);

	pm_runtime_disable(&host->pdev->dev);
	devm_kfree(&pdev->dev, fb_info->pseudo_palette);
fb_release:
	framebuffer_release(fb_info);
	devm_kfree(&pdev->dev, host);

	return ret;
}
```

此函数主要工作:

- 获取硬件需要的内存io资源
- 分配一个host结构
- 分配一个fb_info结构
- 将lcd控制寄存器硬件物理地址映射成虚拟
- 时钟设置
- 初始化fb_info
- 激活lcd
- 向内核注册lcd设备

 probe中调用mxsfb_init_fbinfo初始化fb_info

```cpp
static int mxsfb_init_fbinfo(struct mxsfb_info *host)
{
	int ret;
	struct fb_info *fb_info = host->fb_info;
	struct fb_var_screeninfo *var = &fb_info->var;//var是fb_info的可变参数
	struct fb_modelist *modelist;

	fb_info->fbops = &mxsfb_ops;//绑定fops，应用操作设备时会被调用
	fb_info->flags = FBINFO_FLAG_DEFAULT | FBINFO_READS_FAST;
	//固定信息fix
	fb_info->fix.type = FB_TYPE_PACKED_PIXELS;
	fb_info->fix.ypanstep = 1;
	fb_info->fix.ywrapstep = 1;
	fb_info->fix.visual = FB_VISUAL_TRUECOLOR,
	fb_info->fix.accel = FB_ACCEL_NONE;

	ret = mxsfb_init_fbinfo_dt(host);//设置fb_info
	if (ret)
		return ret;

	if (host->id < 0)
		sprintf(fb_info->fix.id, "mxs-lcdif");
	else
		sprintf(fb_info->fix.id, "mxs-lcdif%d", host->id);

	if (!list_empty(&fb_info->modelist)) {
		/* first video mode in the modelist as default video mode  */
		modelist = list_first_entry(&fb_info->modelist,
				struct fb_modelist, list);
		fb_videomode_to_var(var, &modelist->mode);//使用modelist中保存的fb_videomode初始化var
	}
	/* save the sync value getting from dtb */
	host->sync = fb_info->var.sync;

	var->nonstd = 0;
	var->activate = FB_ACTIVATE_NOW;
	var->accel_flags = 0;
	var->vmode = FB_VMODE_NONINTERLACED;

	/* init the color fields */
	mxsfb_check_var(var, fb_info);//设置fb_info的bitfield,设置var

	fb_info->fix.line_length =
		fb_info->var.xres * (fb_info->var.bits_per_pixel >> 3);//显存每行的长度
	fb_info->fix.smem_len = SZ_32M;//显存大小

	/* Memory allocation for framebuffer */
	if (mxsfb_map_videomem(fb_info) < 0)//从dma分配并清空显存
		return -ENOMEM;

	if (mxsfb_restore_mode(host))
		memset((char *)fb_info->screen_base, 0, fb_info->fix.smem_len);

	return 0;
}


static int mxsfb_init_fbinfo_dt(struct mxsfb_info *host)
{
	struct fb_info *fb_info = host->fb_info;
	struct fb_var_screeninfo *var = &fb_info->var;
	struct device *dev = &host->pdev->dev;
	struct device_node *np = host->pdev->dev.of_node;
	struct device_node *display_np;
	struct device_node *timings_np;
	struct display_timings *timings = NULL;
	const char *disp_dev, *disp_videomode;
	u32 width;
	int i;
	int ret = 0;

	host->id = of_alias_get_id(np, "lcdif");

	display_np = of_parse_phandle(np, "display", 0);
	if (!display_np) {
		dev_err(dev, "failed to find display phandle\n");
		return -ENOENT;
	}

	ret = of_property_read_u32(display_np, "bus-width", &width);
	if (ret < 0) {
		dev_err(dev, "failed to get property bus-width\n");
		goto put_display_node;
	}

	switch (width) {
	case 8:
		host->ld_intf_width = STMLCDIF_8BIT;
		break;
	case 16:
		host->ld_intf_width = STMLCDIF_16BIT;
		break;
	case 18:
		host->ld_intf_width = STMLCDIF_18BIT;
		break;
	case 24:
		host->ld_intf_width = STMLCDIF_24BIT;
		break;
	default:
		dev_err(dev, "invalid bus-width value\n");
		ret = -EINVAL;
		goto put_display_node;
	}

	ret = of_property_read_u32(display_np, "bits-per-pixel",
				   &var->bits_per_pixel);//像素类型16bpp 24bpp...
	if (ret < 0) {
		dev_err(dev, "failed to get property bits-per-pixel\n");
		goto put_display_node;
	}

	ret = of_property_read_string(np, "disp-dev", &disp_dev);
	if (!ret) {
		memcpy(host->disp_dev, disp_dev, strlen(disp_dev));

		if (!of_property_read_string(np, "disp-videomode",
					    &disp_videomode)) {
			memcpy(host->disp_videomode, disp_videomode,
			       strlen(disp_videomode));
		}

		/* Timing is from encoder driver */
		goto put_display_node;
	}

	timings = of_get_display_timings(display_np);//解析设备树获得display_timing[]
	if (!timings) {
		dev_err(dev, "failed to get display timings\n");
		ret = -ENOENT;
		goto put_display_node;
	}

	timings_np = of_find_node_by_name(display_np,
					  "display-timings");
	if (!timings_np) {
		dev_err(dev, "failed to find display-timings node\n");
		ret = -ENOENT;
		goto put_display_node;
	}

	for (i = 0; i < of_get_child_count(timings_np); i++) {
		struct videomode vm;
		struct fb_videomode fb_vm;

		ret = videomode_from_timings(timings, &vm, i);//遍历display_timing,将display_timing中的值给到vm
		if (ret < 0)
			goto put_timings_node;
		ret = fb_videomode_from_videomode(&vm, &fb_vm);//将vm的值给到fb_vm
		if (ret < 0)
			goto put_timings_node;

		if (!(vm.flags & DISPLAY_FLAGS_DE_HIGH))
			fb_vm.sync |= FB_SYNC_OE_LOW_ACT;
		if (vm.flags & DISPLAY_FLAGS_PIXDATA_NEGEDGE)
			fb_vm.sync |= FB_SYNC_CLK_LAT_FALL;
		fb_add_videomode(&fb_vm, &fb_info->modelist);//将fb_vm添加到modelist链表
	}

put_timings_node:
	of_node_put(timings_np);
put_display_node:
	if (timings)
		kfree(timings);
	of_node_put(display_np);
	return ret;
}


struct display_timings *of_get_display_timings(struct device_node *np)
{
	struct device_node *timings_np;
	struct device_node *entry;
	struct device_node *native_mode;
	struct display_timings *disp;

	if (!np)
		return NULL;

	timings_np = of_get_child_by_name(np, "display-timings");//解析display-timings节点
	if (!timings_np) {
		pr_err("%s: could not find display-timings node\n",
			of_node_full_name(np));
		return NULL;
	}

	disp = kzalloc(sizeof(*disp), GFP_KERNEL);//分配一个display_timings
	if (!disp) {
		pr_err("%s: could not allocate struct disp'\n",
			of_node_full_name(np));
		goto dispfail;
	}

	entry = of_parse_phandle(timings_np, "native-mode", 0);//解析"native-mode"
	/* assume first child as native mode if none provided */
	if (!entry)
		entry = of_get_next_child(timings_np, NULL);
	/* if there is no child, it is useless to go on */
	if (!entry) {
		pr_err("%s: no timing specifications given\n",
			of_node_full_name(np));
		goto entryfail;
	}

	pr_debug("%s: using %s as default timing\n",
		of_node_full_name(np), entry->name);

	native_mode = entry;

	disp->num_timings = of_get_child_count(timings_np);//获取timing节点个数
	if (disp->num_timings == 0) {
		/* should never happen, as entry was already found above */
		pr_err("%s: no timings specified\n", of_node_full_name(np));
		goto entryfail;
	}

	disp->timings = kzalloc(sizeof(struct display_timing *) *
				disp->num_timings, GFP_KERNEL);//分配display_timing[]
	if (!disp->timings) {
		pr_err("%s: could not allocate timings array\n",
			of_node_full_name(np));
		goto entryfail;
	}

	disp->num_timings = 0;
	disp->native_mode = 0;

	for_each_child_of_node(timings_np, entry) {//遍历所有的timing
		struct display_timing *dt;
		int r;

		dt = kzalloc(sizeof(*dt), GFP_KERNEL);
		if (!dt) {
			pr_err("%s: could not allocate display_timing struct\n",
					of_node_full_name(np));
			goto timingfail;
		}

		r = of_parse_display_timing(entry, dt);//解析timing，将解析结果给到dt
		if (r) {
			/*
			 * to not encourage wrong devicetrees, fail in case of
			 * an error
			 */
			pr_err("%s: error in timing %d\n",
				of_node_full_name(np), disp->num_timings + 1);
			kfree(dt);
			goto timingfail;
		}

		if (native_mode == entry)
			disp->native_mode = disp->num_timings;

		disp->timings[disp->num_timings] = dt;//将dt给到display_timing[i]
		disp->num_timings++;
	}
	of_node_put(timings_np);
	/*
	 * native_mode points to the device_node returned by of_parse_phandle
	 * therefore call of_node_put on it
	 */
	of_node_put(native_mode);

	pr_debug("%s: got %d timings. Using timing #%d as default\n",
		of_node_full_name(np), disp->num_timings,
		disp->native_mode + 1);

	return disp;

timingfail:
	of_node_put(native_mode);
	display_timings_release(disp);
	disp = NULL;
entryfail:
	kfree(disp);
dispfail:
	of_node_put(timings_np);
	return NULL;
}

static int of_parse_display_timing(const struct device_node *np,
		struct display_timing *dt)
{
	u32 val = 0;
	int ret = 0;

	memset(dt, 0, sizeof(*dt));
	//解析dts节点，保存结果
	ret |= parse_timing_property(np, "hback-porch", &dt->hback_porch);
	ret |= parse_timing_property(np, "hfront-porch", &dt->hfront_porch);
	ret |= parse_timing_property(np, "hactive", &dt->hactive);
	ret |= parse_timing_property(np, "hsync-len", &dt->hsync_len);
	ret |= parse_timing_property(np, "vback-porch", &dt->vback_porch);
	ret |= parse_timing_property(np, "vfront-porch", &dt->vfront_porch);
	ret |= parse_timing_property(np, "vactive", &dt->vactive);
	ret |= parse_timing_property(np, "vsync-len", &dt->vsync_len);
	ret |= parse_timing_property(np, "clock-frequency", &dt->pixelclock);

	dt->flags = 0;
	if (!of_property_read_u32(np, "vsync-active", &val))
		dt->flags |= val ? DISPLAY_FLAGS_VSYNC_HIGH :
				DISPLAY_FLAGS_VSYNC_LOW;
	if (!of_property_read_u32(np, "hsync-active", &val))
		dt->flags |= val ? DISPLAY_FLAGS_HSYNC_HIGH :
				DISPLAY_FLAGS_HSYNC_LOW;
	if (!of_property_read_u32(np, "de-active", &val))
		dt->flags |= val ? DISPLAY_FLAGS_DE_HIGH :
				DISPLAY_FLAGS_DE_LOW;
	if (!of_property_read_u32(np, "pixelclk-active", &val))
		dt->flags |= val ? DISPLAY_FLAGS_PIXDATA_POSEDGE :
				DISPLAY_FLAGS_PIXDATA_NEGEDGE;

	if (of_property_read_bool(np, "interlaced"))
		dt->flags |= DISPLAY_FLAGS_INTERLACED;
	if (of_property_read_bool(np, "doublescan"))
		dt->flags |= DISPLAY_FLAGS_DOUBLESCAN;
	if (of_property_read_bool(np, "doubleclk"))
		dt->flags |= DISPLAY_FLAGS_DOUBLECLK;

	if (ret) {
		pr_err("%s: error reading timing properties\n",
			of_node_full_name(np));
		return -EINVAL;
	}

	return 0;
}

static int mxsfb_restore_mode(struct mxsfb_info *host)
{
	struct fb_info *fb_info = host->fb_info;
	unsigned line_count;
	unsigned period;
	unsigned long pa, fbsize;
	int bits_per_pixel, ofs;
	u32 transfer_count, vdctrl0, vdctrl2, vdctrl3, vdctrl4, ctrl;
	struct fb_videomode vmode;

	clk_enable_axi(host);//使能时钟
	clk_enable_disp_axi(host);

#ifndef CONFIG_FB_IMX64_DEBUG
	/* Enable pixel clock earlier since in 7D
	 * the lcdif registers should be accessed
	 * when the pixel clock is enabled, otherwise
	 * the bus will be hang.
	 */
	clk_enable_pix(host);
#endif

	/* Only restore the mode when the controller is running */
	ctrl = readl(host->base + LCDC_CTRL);//读取lcd运行状态
	if (!(ctrl & CTRL_RUN))
		return -EINVAL;

	memset(&vmode, 0, sizeof(vmode));

	vdctrl0 = readl(host->base + LCDC_VDCTRL0);//读取lcd寄存器
	vdctrl2 = readl(host->base + LCDC_VDCTRL2);
	vdctrl3 = readl(host->base + LCDC_VDCTRL3);
	vdctrl4 = readl(host->base + LCDC_VDCTRL4);

	transfer_count = readl(host->base + host->devdata->transfer_count);

	vmode.xres = TRANSFER_COUNT_GET_HCOUNT(transfer_count);//获取硬件lcd分辨率
	vmode.yres = TRANSFER_COUNT_GET_VCOUNT(transfer_count);

	switch (CTRL_GET_WORD_LENGTH(ctrl)) {
	case 0:
		bits_per_pixel = 16;
		break;
	case 3:
		bits_per_pixel = 32;
		break;
	case 1:
	default:
		return -EINVAL;
	}

	fb_info->var.bits_per_pixel = bits_per_pixel;//重设fb_info

	vmode.pixclock = KHZ2PICOS(clk_get_rate(host->clk_pix) / 1000U);
	vmode.hsync_len = get_hsync_pulse_width(host, vdctrl2);
	vmode.left_margin = GET_HOR_WAIT_CNT(vdctrl3) - vmode.hsync_len;
	vmode.right_margin = VDCTRL2_GET_HSYNC_PERIOD(vdctrl2) - vmode.hsync_len -
		vmode.left_margin - vmode.xres;
	vmode.vsync_len = VDCTRL0_GET_VSYNC_PULSE_WIDTH(vdctrl0);
	period = readl(host->base + LCDC_VDCTRL1);
	vmode.upper_margin = GET_VERT_WAIT_CNT(vdctrl3) - vmode.vsync_len;
	vmode.lower_margin = period - vmode.vsync_len - vmode.upper_margin - vmode.yres;

	vmode.vmode = FB_VMODE_NONINTERLACED;

	vmode.sync = 0;
	if (vdctrl0 & VDCTRL0_HSYNC_ACT_HIGH)
		vmode.sync |= FB_SYNC_HOR_HIGH_ACT;
	if (vdctrl0 & VDCTRL0_VSYNC_ACT_HIGH)
		vmode.sync |= FB_SYNC_VERT_HIGH_ACT;

	pr_debug("Reconstructed video mode:\n");
	pr_debug("%dx%d, hsync: %u left: %u, right: %u, vsync: %u, upper: %u, lower: %u\n",
			vmode.xres, vmode.yres,
			vmode.hsync_len, vmode.left_margin, vmode.right_margin,
			vmode.vsync_len, vmode.upper_margin, vmode.lower_margin);
	pr_debug("pixclk: %ldkHz\n", PICOS2KHZ(vmode.pixclock));

	fb_add_videomode(&vmode, &fb_info->modelist);//将从lcd读取的数据组成fb_videomode保存到modelist

	host->ld_intf_width = CTRL_GET_BUS_WIDTH(ctrl);
	host->dotclk_delay = VDCTRL4_GET_DOTCLK_DLY(vdctrl4);

	fb_info->fix.line_length = vmode.xres * (bits_per_pixel >> 3);

	pa = readl(host->base + host->devdata->cur_buf);
	fbsize = fb_info->fix.line_length * vmode.yres;
	if (pa < fb_info->fix.smem_start)
		return -EINVAL;
	if (pa + fbsize > fb_info->fix.smem_start + fb_info->fix.smem_len)
		return -EINVAL;
	ofs = pa - fb_info->fix.smem_start;
	if (ofs) {
		memmove(fb_info->screen_base, fb_info->screen_base + ofs, fbsize);
		writel(fb_info->fix.smem_start, host->base + host->devdata->next_buf);
	}

	line_count = fb_info->fix.smem_len / fb_info->fix.line_length;
	fb_info->fix.ypanstep = 1;
	fb_info->fix.ywrapstep = 1;

	host->enabled = 1;//

	return 0;
}
```

mxsfb_init_fbinfo主要工作：

- 解析设备树，填充fb_info结构体，包括可变信息var,固定信息fix...
- 绑定fops结构体
- 从dma分配显存
- 使能时钟，开启lcd

如下是fops结构体，当用户打开lcd设备时，会调用fops中的函数

```cpp
static struct fb_ops mxsfb_ops = {
	.owner = THIS_MODULE,
	.fb_check_var = mxsfb_check_var,
	.fb_set_par = mxsfb_set_par,
	.fb_setcolreg = mxsfb_setcolreg,
	.fb_ioctl = mxsfb_ioctl,
	.fb_blank = mxsfb_blank,
	.fb_pan_display = mxsfb_pan_display,
	.fb_mmap = mxsfb_mmap,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
};
```

ioctl函数：

```cpp
static int mxsfb_ioctl(struct fb_info *fb_info, unsigned int cmd,
			unsigned long arg)
{
	int ret = -EINVAL;

	switch (cmd) {
	case MXCFB_WAIT_FOR_VSYNC:
		ret = mxsfb_wait_for_vsync(fb_info);//具有超时的等待完成量
		break;
	default:
		break;
	}
	return ret;
}

static int mxsfb_wait_for_vsync(struct fb_info *fb_info)
{
	struct mxsfb_info *host = fb_info->par;
	int ret = 0;

	if (host->cur_blank != FB_BLANK_UNBLANK) {
		dev_err(fb_info->device, "can't wait for VSYNC when fb "
			"is blank\n");
		return -EINVAL;
	}

	init_completion(&host->vsync_complete);

	host->wait4vsync = 1;
	writel(CTRL1_VSYNC_EDGE_IRQ_EN,
		host->base + LCDC_CTRL1 + REG_SET);//使能vsync中断
	ret = wait_for_completion_interruptible_timeout(
				&host->vsync_complete, 1 * HZ);
	if (ret == 0) {
		dev_err(fb_info->device,
			"mxs wait for vsync timeout\n");
		host->wait4vsync = 0;
		ret = -ETIME;
	} else if (ret > 0) {
		ret = 0;
	}
	return ret;
}
```

在probe函数中会注册一个中断：

```cpp
ret = devm_request_irq(&pdev->dev, irq, mxsfb_irq_handler, 0,
			  dev_name(&pdev->dev), host);//申请中断


static irqreturn_t mxsfb_irq_handler(int irq, void *dev_id)
{
	struct mxsfb_info *host = dev_id;
	u32 ctrl1, enable, status, acked_status;

	ctrl1 = readl(host->base + LCDC_CTRL1);
	enable = (ctrl1 & CTRL1_IRQ_ENABLE_MASK) >> CTRL1_IRQ_ENABLE_SHIFT;
	status = (ctrl1 & CTRL1_IRQ_STATUS_MASK) >> CTRL1_IRQ_STATUS_SHIFT;
	acked_status = (enable & status) << CTRL1_IRQ_STATUS_SHIFT;

	if ((acked_status & CTRL1_VSYNC_EDGE_IRQ) && host->wait4vsync) {
		writel(CTRL1_VSYNC_EDGE_IRQ,
				host->base + LCDC_CTRL1 + REG_CLR);
		writel(CTRL1_VSYNC_EDGE_IRQ_EN,
			     host->base + LCDC_CTRL1 + REG_CLR);
		host->wait4vsync = 0;
		complete(&host->vsync_complete);
	}

	if (acked_status & CTRL1_CUR_FRAME_DONE_IRQ) {
		writel(CTRL1_CUR_FRAME_DONE_IRQ,
				host->base + LCDC_CTRL1 + REG_CLR);
		writel(CTRL1_CUR_FRAME_DONE_IRQ_EN,
			     host->base + LCDC_CTRL1 + REG_CLR);
		complete(&host->flip_complete);//中断处理程序中返回完成量，解除ioctl的阻塞
	}

	if (acked_status & CTRL1_UNDERFLOW_IRQ)
		writel(CTRL1_UNDERFLOW_IRQ, host->base + LCDC_CTRL1 + REG_CLR);

	if (acked_status & CTRL1_OVERFLOW_IRQ)
		writel(CTRL1_OVERFLOW_IRQ, host->base + LCDC_CTRL1 + REG_CLR);

	return IRQ_HANDLED;
}
```

mmap函数：

```cpp
static int mxsfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	u32 len;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

	if (offset < info->fix.smem_len) {
		/* mapping framebuffer memory */
		len = info->fix.smem_len - offset;
		vma->vm_pgoff = (info->fix.smem_start + offset) >> PAGE_SHIFT;
	} else
		return -EINVAL;

	len = PAGE_ALIGN(len);
	if (vma->vm_end - vma->vm_start > len)
		return -EINVAL;

	/* make buffers bufferable */
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
			    vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		dev_dbg(info->device, "mmap remap_pfn_range failed\n");
		return -ENOBUFS;
	}

	return 0;
}
```

### probe函数中调用register_framebuffer

```cpp
int
register_framebuffer(struct fb_info *fb_info)
{
	int ret;

	mutex_lock(&registration_lock);
	ret = do_register_framebuffer(fb_info);
	mutex_unlock(&registration_lock);

	return ret;
}

static int do_register_framebuffer(struct fb_info *fb_info)
{
	int i, ret;
	struct fb_event event;
	struct fb_videomode mode;

	if (fb_check_foreignness(fb_info))
		return -ENOSYS;

	ret = do_remove_conflicting_framebuffers(fb_info->apertures,
						 fb_info->fix.id,
						 fb_is_primary_device(fb_info));
	if (ret)
		return ret;

	if (num_registered_fb == FB_MAX)
		return -ENXIO;

	num_registered_fb++;
	for (i = 0 ; i < FB_MAX; i++)
		if (!registered_fb[i])
			break;
	fb_info->node = i;
	atomic_set(&fb_info->count, 1);
	mutex_init(&fb_info->lock);
	mutex_init(&fb_info->mm_lock);

	fb_info->dev = device_create(fb_class, fb_info->device,
				     MKDEV(FB_MAJOR, i), NULL, "fb%d", i);//调用device_create创建设备节点
	if (IS_ERR(fb_info->dev)) {
		/* Not fatal */
		printk(KERN_WARNING "Unable to create device for framebuffer %d; errno = %ld\n", i, PTR_ERR(fb_info->dev));
		fb_info->dev = NULL;
	} else
		fb_init_device(fb_info);//创建属性文件

	if (fb_info->pixmap.addr == NULL) {
		fb_info->pixmap.addr = kmalloc(FBPIXMAPSIZE, GFP_KERNEL);
		if (fb_info->pixmap.addr) {
			fb_info->pixmap.size = FBPIXMAPSIZE;
			fb_info->pixmap.buf_align = 1;
			fb_info->pixmap.scan_align = 1;
			fb_info->pixmap.access_align = 32;
			fb_info->pixmap.flags = FB_PIXMAP_DEFAULT;
		}
	}	
	fb_info->pixmap.offset = 0;

	if (!fb_info->pixmap.blit_x)
		fb_info->pixmap.blit_x = ~(u32)0;

	if (!fb_info->pixmap.blit_y)
		fb_info->pixmap.blit_y = ~(u32)0;

	if (!fb_info->modelist.prev || !fb_info->modelist.next)
		INIT_LIST_HEAD(&fb_info->modelist);

	if (fb_info->skip_vt_switch)
		pm_vt_switch_required(fb_info->dev, false);
	else
		pm_vt_switch_required(fb_info->dev, true);

	fb_var_to_videomode(&mode, &fb_info->var);//var->mode
	fb_add_videomode(&mode, &fb_info->modelist);//mode->modelist
	registered_fb[i] = fb_info;//保存fb_info到全局变量

	event.info = fb_info;
	if (!lockless_register_fb)
		console_lock();
	if (!lock_fb_info(fb_info)) {
		if (!lockless_register_fb)
			console_unlock();
		return -ENODEV;
	}

	fb_notifier_call_chain(FB_EVENT_FB_REGISTERED, &event);
	unlock_fb_info(fb_info);
	if (!lockless_register_fb)
		console_unlock();
	return 0;
}
```

register_framebuffer向内核注册一个fb_info，生成设备节点

## 以下内容来自wds

根据probe函数分析，对于实际的lcd硬件，主要需要为lcd设置如下参数：

* GPIO设置
  * LCD引脚
  * 背光引脚
* 时钟设置
  * 确定LCD控制器的时钟
  * 根据LCD的DCLK计算相关时钟
* LCD控制器本身的设置
  * 比如设置Framebuffer的地址
  * 设置Framebuffer中数据格式、LCD数据格式
  * 设置时序

#### 2.1 GPIO设置

有两种方法：

* 直接读写相关寄存器
* 使用设备树，在设备树中设置pinctrl
  * 本课程专注于LCD，所以使用pinctrl简化程序

设备树`arch/arm/boot/dts/100ask_imx6ull-14x14.dts`中：
![image-20230527013357831](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230527013357831.png)





#### 2.2 时钟设置

IMX6ULL的LCD控制器涉及2个时钟：

![image-20230527013315499](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230527013315499.png)

代码里直接使用时钟子系统的代码。

* 在设备树里指定频率：

  * 文件：arch/arm/boot/dts/100ask_imx6ull-14x14.dts

  * 代码：clock-frequency

    ```shell
           display-timings {
                native-mode = <&timing0>;
    
                 timing0: timing0_1024x768 {
                 clock-frequency = <50000000>;
    ```

    

* 从设备树获得dot clock，存入display_timing

  * 文件：drivers\video\of_display_timing.c

  * 代码：

    ```c
    ret |= parse_timing_property(np, "clock-frequency", &dt->pixelclock);
    ```

* 使用display_timing来设置videomode

  * 文件：drivers\video\videomode.c

  * 代码：

    ```c
    void videomode_from_timing(const struct display_timing *dt,
    			  struct videomode *vm)
    {
    	vm->pixelclock = dt->pixelclock.typ;
    	vm->hactive = dt->hactive.typ;
    	vm->hfront_porch = dt->hfront_porch.typ;
    	vm->hback_porch = dt->hback_porch.typ;
    	vm->hsync_len = dt->hsync_len.typ;
    
    	vm->vactive = dt->vactive.typ;
    	vm->vfront_porch = dt->vfront_porch.typ;
    	vm->vback_porch = dt->vback_porch.typ;
    	vm->vsync_len = dt->vsync_len.typ;
    
    	vm->flags = dt->flags;
    }
    
    ```

    

* 根据videomode的值，使用时钟子系统的函数设置时钟：

  * 文件：drivers\video\fbdev\mxc\ldb.c
  * 代码：
    ![image-20230527013252455](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230527013252455.png)

  

#### 2.3 LCD控制器的配置

以设置分辨率为例。

* 在设备树里指定频率：

  * 文件：arch/arm/boot/dts/100ask_imx6ull-14x14.dts

  * 代码：clock-frequency

    ```shell
           display-timings {
                native-mode = <&timing0>;
    
                 timing0: timing0_1024x768 {
    				hactive = <1024>;
    	            vactive = <600>;
    
    ```

* 从设备树获得分辨率，存入display_timing

  * 文件：drivers\video\of_display_timing.c

  * 代码：

    ```c
    	ret |= parse_timing_property(np, "hactive", &dt->hactive);
    	ret |= parse_timing_property(np, "vactive", &dt->vactive);
    ```

* 使用display_timing来设置videomode

  * 文件：drivers\video\videomode.c

  * 代码：

    ```c
    void videomode_from_timing(const struct display_timing *dt,
    			  struct videomode *vm)
    {
    	vm->hactive = dt->hactive.typ;
    
        vm->vactive = dt->vactive.typ;
    ```

    

* 根据videomode的值，设置fb_videomode

  * 文件：drivers\video\fbdev\core\fbmon.c

  * 代码：

    ```c
    int fb_videomode_from_videomode(const struct videomode *vm,
    				struct fb_videomode *fbmode)
    {
    	unsigned int htotal, vtotal;
    
    	fbmode->xres = vm->hactive;
    
        fbmode->yres = vm->vactive;
    
    ```

* 根据fb_videomode的值，设置fb_info中的var：

  * 文件：drivers\video\fbdev\core\modedb.c

  * 代码：

    ```c
    void fb_videomode_to_var(struct fb_var_screeninfo *var,
    			 const struct fb_videomode *mode)
    {
    	var->xres = mode->xres;
    	var->yres = mode->yres;
    
    ```

    

* 根据var的分辨率，设置寄存器

  * 文件：drivers\video\fbdev\mxsfb.c

  * 代码：

  ```cpp
  	writel(TRANSFER_COUNT_SET_VCOUNT(fb_info->var.yres) |
  			TRANSFER_COUNT_SET_HCOUNT(fb_info->var.xres),
  			host->base + host->devdata->transfer_count);
  ```

  