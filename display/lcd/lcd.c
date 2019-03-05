/*
 * kernel/aml-4.9/drivers/amlogic/lcd/lcd.c
 *
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/amlogic/pm.h>

#include <linux/regulator/consumer.h>
#include <linux/types.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/suspend.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/ioctl.h>

#include "../../gpio/gpiolib.h"

#include "lcd.h"

#define SUPPORT_ST7567

/* 液晶128*64 */
#define LCD_WIDTH 128
#define LCD_HEIGHT 64

#define GPIO_DIRECTION_INPUT 0
#define GPIO_DIRECTION_OUTPUT 1

#define START_COL_OFFSET 0x04
#define START_ROW_OFFSET 0x10

#define LABEL_kKELCD_BL   "lcd-spi-bl"
#define LABEL_kKELCD_CS   "lcd-spi-cs"
#define LABEL_kKELCD_A0   "lcd-spi-a0"
#define LABEL_kKELCD_RST  "lcd-spi-rst"
#define LABEL_kKELCD_CLK  "lcd-spi-clk"
#define LABEL_kKELCD_DATA "lcd-spi-data"

#define ke_gpio_setpin(gpio, value) gpio_set_value(gpio, value)
#define ke_gpio_getpin(gpio) gpio_get_value(gpio)

int gpio_lcd_rst = -1;
int gpio_lcd_bl = -1;
int gpio_lcd_cs = -1;
int gpio_lcd_a0 = -1;
int gpio_lcd_clk = -1;
int gpio_lcd_data = -1;

static int lcd_major = 0;
static int lcd_minor = 0;

static struct class *lcd_class = NULL;
static struct lcd_android_dev *lcd_dev = NULL;


//串行模式的写操作
void data_send(unsigned char dat)
{
	int i;
	for (i = 0; i < 8; i++)
	{
		ke_gpio_setpin(gpio_lcd_clk, 0);
		ke_gpio_setpin(gpio_lcd_data, (dat >> (7 - i)) & 0x01);
		ke_gpio_setpin(gpio_lcd_clk, 1);
	}
}

//写命
void write_cmd(unsigned char cmd)
{
	ke_gpio_setpin(gpio_lcd_cs, 0); //低电平，使能
	ke_gpio_setpin(gpio_lcd_a0, 0); //低电平，写命
	data_send(cmd);
	ke_gpio_setpin(gpio_lcd_cs, 1); //高电
}

//写数
void write_data(unsigned char dat)
{
	ke_gpio_setpin(gpio_lcd_cs, 0); //低电平，使能
	ke_gpio_setpin(gpio_lcd_a0, 1); //高电平，写数据
	data_send(dat);
	ke_gpio_setpin(gpio_lcd_cs, 1); //高电
}

void back_light(unsigned char dat)
{
	printk("back_light on=%d\n", dat);
	if (dat > 0)
	{
		ke_gpio_setpin(gpio_lcd_bl, 1);
	}
	else
	{
		ke_gpio_setpin(gpio_lcd_bl, 0);
	}
}

void lcd_close(void)
{
	write_cmd(0xae);
	write_cmd(0xe2);

	ke_gpio_setpin(gpio_lcd_cs, 1);
	mdelay(3);
	ke_gpio_setpin(gpio_lcd_cs, 0);
}

void lcd_clean(void)
{
	unsigned char n, m;
	for (n = 0; n < 9; n++)
	{
		write_cmd(0xB0 + n); // Setting Page 0 -- 7
		write_cmd(0x10);	 // Column Address Ser Upper.
#ifdef	SUPPORT_ST7567
		write_cmd(0x04);	 //write_cmd(0x04);	// Column Address Ser Lower.  00 is first Column
#else
		write_cmd(0x00);
#endif
		for (m = 0; m < LCD_WIDTH; m++)
		{
			write_data(0x0);
		}
	}
}

void lcd_set_column(unsigned char col)
{
	unsigned char lsb;

	lsb = (col & 0xF);
	col = (col >> 4) & 0xF;

	// send  msb
	write_cmd(col | 0x10);
	// send  lsb
	write_cmd(lsb);
}

void lcd_set_page(unsigned char page)
{
	unsigned char cmd;

	cmd = 0xB0 + (page & 0xF);
	write_cmd(cmd);
}

static ssize_t lcd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t err = 0;
	struct lcd_android_dev *dev = filp->private_data;
	int val = 0;

	pr_info("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	if (down_interruptible(&(dev->sem)))
	{
		return -ERESTARTSYS;
	}

	if (copy_to_user(buf, &val, sizeof(int)))
	{
		err = -EFAULT;
		return -1;
	}
	up(&(dev->sem));
	return err;
}

static ssize_t lcd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct lcd_android_dev *dev = filp->private_data;
	ssize_t err = 0;
	char value = 0;

	pr_info("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	if (down_interruptible(&(dev->sem)))
	{
		return -ERESTARTSYS;
	}

	if (copy_from_user(&value, buf, count))
	{
		err = -EFAULT;
		return -1;
	}

	up(&(dev->sem));
	return err;
}

static long lcd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int val = 0;
	int reg_val = 0;

	pr_info("kelcd_ioctl cmd=%d arg=%ld\n", cmd, arg);
	switch (cmd)
	{
		case GPIO_LCD_WRITE_CMD:
			val = *(unsigned int *)arg;
			reg_val = val & 0xff;
			write_cmd(reg_val);
			break;

		case GPIO_LCD_WRITE_DATA:
			val = *(unsigned int *)arg;
			reg_val = val & 0xff;
			write_data(reg_val);
			break;

		case GPIO_LCD_CLOSE:
			pr_info("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
			break;

		case GPIO_LCD_OPEN:
			pr_info("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
			break;

		case GPIO_LCD_BACKLIGHT:
			pr_info("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
			val = *(unsigned int *)arg;
			reg_val = val & 0xff;
			back_light(reg_val);
			break;

		default:
			return -1;
	}

	return ret;
}

static int lcd_release(struct inode *inode, struct file *filp)
{
	pr_info("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

static int lcd_open(struct inode *inode, struct file *filp)
{
	struct lcd_android_dev *dev;

	dev = container_of(inode->i_cdev, struct lcd_android_dev, dev);
	filp->private_data = dev;
	pr_info("open lcd device okay!\n");
	
	return 0;
}

static struct file_operations lcd_fops = {
	.owner = THIS_MODULE,
	.open = lcd_open,
	.release = lcd_release,
	.read = lcd_read,
	.write = lcd_write,
//	.unlocked_ioctl = kelcd_ioctl,
	.compat_ioctl = lcd_ioctl,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////
void hw_lcd_update(void)
{
	/* nothing for none-DMA mode driver */
	unsigned char *pLcdbuf = (unsigned char *)RES_ICON_TIETONG_LOGO;
	unsigned char line, col;

	for(line = 0; line < 8; line++)
	{
#ifdef	SUPPORT_ST7567
		// set page address
		lcd_set_page(line);
		write_cmd(START_ROW_OFFSET);
#else
		// set display start line
		write_cmd(0x0);
		// set page
		lcd_set_page(line);
#endif
		// set start col
		lcd_set_column(START_COL_OFFSET);

		for (col = 0; col < LCD_WIDTH; col++)
		{
			write_data(*pLcdbuf);
			pLcdbuf++;
		}
	}
}

void lcd_init_code(void)
{
#ifdef	SUPPORT_ST7567
	write_cmd(0xe2); // reset signal
	mdelay(1);
	write_cmd(0xa2); // 1/9 bias
	write_cmd(0xa1); // ADC select
	write_cmd(0xc0); // common output select
	write_cmd(0x40); // display start line
	write_cmd(0x2f); // power control(Booster: ON  regulator : ON	follower: ON)
	mdelay(1);
	write_cmd(0xf8); // Booster Ratio Select
	write_cmd(0x00); // 2x,3x,4x
	write_cmd(0x26); // select resistor ratio Rb/Ra
	write_cmd(0x81); // select volume
	write_cmd(0x17); // vop
	write_cmd(0xaf); //display on
#else
	write_cmd(0x00);
	write_cmd(0xaf);
	write_cmd(0xe2);
	write_cmd(0x2c);
	write_cmd(0x2e);
	write_cmd(0x2f);
	write_cmd(0xa0);
	write_cmd(0xc8);
	write_cmd(0x40 + 0x20);
	write_cmd(0xa6);
	write_cmd(0x25);
	write_cmd(0x81);
	write_cmd(0x20);
	write_cmd(0xaf);
#endif
}

void lcd_gpio_init(void)
{
	//液晶复位
	gpio_direction_output(gpio_lcd_rst, GPIO_DIRECTION_OUTPUT);

	//液晶片选
	gpio_direction_output(gpio_lcd_cs, GPIO_DIRECTION_OUTPUT);

	//液晶A0
	gpio_direction_output(gpio_lcd_a0, GPIO_DIRECTION_OUTPUT);

	//液晶时钟
	gpio_direction_output(gpio_lcd_clk, GPIO_DIRECTION_OUTPUT);

	//液晶数据
	gpio_direction_output(gpio_lcd_data, GPIO_DIRECTION_OUTPUT);

	//液晶背光
	gpio_direction_output(gpio_lcd_bl, GPIO_DIRECTION_OUTPUT);
}

static int lcd_parse_dts(struct platform_device *pdev)
{
	enum of_gpio_flags flags;
	struct device_node *node;
	int setGpio = -1;
	int error = -1;

	node = pdev->dev.of_node;

	setGpio = of_get_named_gpio_flags(node, LABEL_kKELCD_BL, 0, &flags);
	if (gpio_is_valid(setGpio))
	{
		error = devm_gpio_request(&pdev->dev, setGpio, "lcd_bl");
		if (error)
		{
			dev_err(&pdev->dev, "failed to request gpio_lcd_bl  %d: %d\n", setGpio, error);
		}
		gpio_lcd_bl = setGpio;
	}
	else
	{
		dev_err(&pdev->dev, "lcd failed gpio gpio_lcd_bl= %d \n", setGpio);
	}

	setGpio = of_get_named_gpio_flags(node, LABEL_kKELCD_CS, 0, &flags);
	if (gpio_is_valid(setGpio))
	{
		error = gpio_request(setGpio, "lcd_cs");
		if (error < 0)
		{
			dev_err(&pdev->dev, "Failed to request gpio_lcd_cs GPIO %d, error %d\n", setGpio, error);
		}
		gpio_lcd_cs = setGpio;
	}
	else
	{
		dev_err(&pdev->dev, "lcd failed gpio gpio_lcd_cs= %d \n", setGpio);
	}

	setGpio = of_get_named_gpio_flags(node, LABEL_kKELCD_RST, 0, &flags);
	if (gpio_is_valid(setGpio))
	{
		error = gpio_request(setGpio, "lcd_rst");
		if (error < 0)
		{
			dev_err(&pdev->dev, "Failed to request gpio_lcd_rst GPIO %d, error %d\n", setGpio, error);
		}
		gpio_lcd_rst = setGpio;
	}
	else
	{
		dev_err(&pdev->dev, "lcd failed gpio gpio_lcd_rst= %d \n", setGpio);
	}
	setGpio = of_get_named_gpio_flags(node, LABEL_kKELCD_A0, 0, &flags);
	if (gpio_is_valid(setGpio))
	{
		error = gpio_request(setGpio, "lcd_a0");
		if (error < 0)
		{
			dev_err(&pdev->dev, "Failed to request gpio_lcd_a0 GPIO %d, error %d\n", setGpio, error);
		}
		gpio_lcd_a0 = setGpio;
	}
	else
	{
		dev_err(&pdev->dev, "lcd failed gpio gpio_lcd_a0= %d \n", setGpio);
	}

	setGpio = of_get_named_gpio_flags(node, LABEL_kKELCD_CLK, 0, &flags);
	if (gpio_is_valid(setGpio))
	{
		error = gpio_request(setGpio, "lcd_clk");
		if (error < 0)
		{
			printk("Failed to request gpio_lcd_clk GPIO %d, error %d\n", setGpio, error);
		}
		gpio_lcd_clk = setGpio;
	}
	else
	{
		printk("lcd failed gpio gpio_lcd_clk= %d \n", setGpio);
	}

	setGpio = of_get_named_gpio_flags(node, LABEL_kKELCD_DATA, 0, &flags);
	if (gpio_is_valid(setGpio))
	{
		error = gpio_request(setGpio, "lcd_data");
		if (error < 0)
		{
			printk("Failed to request gpio_lcd_data GPIO %d, error %d\n", setGpio, error);
		}
		gpio_lcd_data = setGpio;
	}
	else
	{
		printk("kelcd failed gpio gpio_lcd_data= %d \n", setGpio);
	}

	return 0;
}

static int lcd_setup_dev(struct lcd_android_dev *dev)
{
	int err;
	dev_t devno = MKDEV(lcd_major, lcd_minor);

	memset(dev, 0, sizeof(struct lcd_android_dev));

	cdev_init(&(dev->dev), &lcd_fops);
	dev->dev.owner = THIS_MODULE;
	dev->dev.ops = &lcd_fops;

	err = cdev_add(&(dev->dev), devno, 1);
	if (err)
	{
		return err;
	}

	sema_init(&(dev->sem), 1);

	return 0;
}

static int lcd_probe(struct platform_device *pdev)
{
	int err = -1;

	struct device *temp = NULL;
	dev_t dev = 0;

	err = alloc_chrdev_region(&dev, 0, 1, KELCD_DEVICE_NODE_NAME);
	if (err < 0)
	{
		dev_err(&pdev->dev,"Failed to alloc char kelcd_dev region.\n");
		goto fail;
	}

	lcd_major = MAJOR(dev);
	lcd_minor = MINOR(dev);

	lcd_dev = kmalloc(sizeof(struct lcd_android_dev), GFP_KERNEL);
	if (!lcd_dev)
	{
		err = -ENOMEM;
		dev_err(&pdev->dev, "Failed to alloc lcd dev.\n");
		goto unregister;
	}

	err = lcd_setup_dev(lcd_dev);
	if (err)
	{
		dev_err(&pdev->dev, "Failed to setup dev: %d.\n", err);
		goto cleanup;
	}

	lcd_class = class_create(THIS_MODULE, KELCD_DEVICE_CLASS_NAME);
	if (IS_ERR(lcd_class))
	{
		err = PTR_ERR(lcd_class);
		dev_err(&pdev->dev, "Failed to create lcd class.\n");
		goto destroy_cdev;
	}

	temp = device_create(lcd_class, NULL, dev, "%s", KELCD_DEVICE_FILE_NAME);
	if (IS_ERR(temp))
	{
		err = PTR_ERR(temp);
		dev_err(&pdev->dev, "Failed to create lcd device.");
		goto destroy_class;
	}
	dev_set_drvdata(temp, lcd_dev);

	lcd_parse_dts(pdev);
	lcd_gpio_init();

	dev_err(&pdev->dev, "lcd reset.\n");
	ke_gpio_setpin(gpio_lcd_rst, 0);
	mdelay(1);
	ke_gpio_setpin(gpio_lcd_rst, 1);
	mdelay(1);

	dev_err(&pdev->dev, "lcd init config.\n");
	lcd_init_code();
	dev_err(&pdev->dev, "lcd clean screen.\n");
	lcd_clean();
	dev_err(&pdev->dev, "lcd display welcome.\n");
	hw_lcd_update();
	back_light(0);

	return 0;
destroy_class:
	class_destroy(lcd_class);

destroy_cdev:
	cdev_del(&(lcd_dev->dev));

cleanup:
	kfree(lcd_dev);

unregister:
	unregister_chrdev_region(MKDEV(lcd_major, lcd_minor), 1);

fail:
	return err;
}

static int lcd_remove(struct platform_device *pdev)
{
	dev_t devno = MKDEV(lcd_major, lcd_minor);

	printk(KERN_ALERT "Destroy kelcd device.\n");
	flush_scheduled_work();

	if (lcd_class)
	{
		device_destroy(lcd_class, MKDEV(lcd_major, lcd_minor));
		class_destroy(lcd_class);
	}

	if (lcd_dev)
	{
		cdev_del(&(lcd_dev->dev));
		kfree(lcd_dev);
	}

	unregister_chrdev_region(devno, 1);
	return 0;
}

static const struct of_device_id lcd_dt_match[] = {
	{.compatible = "amlogic, lcd-display", },
	{},
};

MODULE_DEVICE_TABLE(of, lcd_dt_match);

static struct platform_driver lcd_driver = {
	.probe = lcd_probe,
	.remove = lcd_remove,
	.driver = {
		.name = "lcd",
		.of_match_table = lcd_dt_match,
	},
};

module_platform_driver(lcd_driver);

MODULE_AUTHOR("Amlogic Liu Ming");
MODULE_DESCRIPTION("lcd drivers");
MODULE_VERSION("2.0.0");
MODULE_LICENSE("GPL");
