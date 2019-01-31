/*
 * drivers/amlogic/input/keyboard/adc_keypad.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
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
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/iio/consumer.h>
#include <linux/input-polldev.h>
#include <linux/amlogic/scpi_protocol.h>
#include <linux/list.h>
#include <linux/input.h>
#include <linux/kobject.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/iio/consumer.h>
#include <dt-bindings/iio/adc/amlogic-saradc.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include "../../../gpio/gpiolib.h"

#define MOER_POWER_OFF			1
#define MOER_IS_POWER_FULL		2
#define MOER_IS_POWER_INSERT	3
#define MOER_START_CHARGE		4
#define MOER_STOP_CHARGE		5
#define MOER_POWER_STATE		6
#define DRIVE_NAME "power_ctl"

struct power_ctl {
	int key_pin[4];
	struct iio_channel *pchan;
};

struct power_ctl *pc;

long power_ctl_ioctl (struct file *file, u32 cmd, unsigned long args){
	int value=0;
	
	switch(args){
		case MOER_POWER_OFF:
			gpio_direction_output(pc->key_pin[2], GPIOF_OUT_INIT_LOW);	
		break;
		case MOER_IS_POWER_FULL:
			//value = gpiod_get_value(gpio_to_desc(pc->key_pin[0]));
		break;
		case MOER_IS_POWER_INSERT:
			value = gpiod_get_value(gpio_to_desc(pc->key_pin[3]));
		break;
		case MOER_START_CHARGE:
			//gpio_direction_output(pc->key_pin[1], GPIOF_OUT_INIT_HIGH);
			//value = gpiod_get_value(gpio_to_desc(pc->key_pin[1]));
		break;
		case MOER_STOP_CHARGE:
			//gpio_direction_output(pc->key_pin[1], GPIOF_OUT_INIT_LOW);
			//value = gpiod_get_value(gpio_to_desc(pc->key_pin[1]));
		break;
		case MOER_POWER_STATE:
			iio_read_channel_processed(pc->pchan,&value);
		break;
	}
	return value;
}

static int power_ctl_open(struct inode *inode,  struct file *file)
{
	file->private_data = pc;
	return 0;
}

static int power_ctl_release(struct inode *inode,  struct file *file)
{
	file->private_data = NULL;
	return 0;
}



static const struct file_operations power_ctl_fops = {
	.owner      = THIS_MODULE,
	.open       = power_ctl_open,
	.release    = power_ctl_release,
	.unlocked_ioctl = power_ctl_ioctl,
};


static int power_ctl_probe(struct platform_device *pdev)
{
	int ret = 0,major = 0;
	int count=0,i=0;
	struct gpio_desc *desc;
	// const char *uname;
	// struct of_phandle_args chanspec;

	pc = kzalloc(sizeof(struct power_ctl), GFP_KERNEL);
	if (!pc)return -ENOMEM;
	

	major = register_chrdev(0, DRIVE_NAME, &power_ctl_fops);
	if (major <= 0) {
		return  major;
	}
	device_create(class_create(THIS_MODULE, DRIVE_NAME), NULL,MKDEV(major, 0), NULL, DRIVE_NAME);

//init adc

	// ret=of_parse_phandle_with_args(pdev->dev.of_node,
	// 		"io-channels", "#io-channel-cells", 0, &chanspec);
	// dev_err(&pdev->dev,"init_adc1:%d\n",ret);
	// ret=of_property_read_string_index(pdev->dev.of_node,
	// 			"io-channel-names", 0, &uname);
	// dev_err(&pdev->dev,"init_adc2:%s--%d\n",uname,ret);

	pc->pchan = devm_iio_channel_get(&pdev->dev,"key-chan-0");

	dev_err(&pdev->dev,"init_adc3:%d\n",IS_ERR(pc->pchan));
//init gpio pin
	of_property_read_u32(pdev->dev.of_node, "key_num", &count);
	for(i=0;i<count;i++){
		desc = of_get_named_gpiod_flags(pdev->dev.of_node,"key_pin",  i,  NULL);
		pc->key_pin[i]=desc_to_gpio(desc);
		gpio_request(pc->key_pin[i],  NULL);
		dev_err(&pdev->dev,"key_pin:%d\n",pc->key_pin[i]);
	}

	gpio_direction_input(pc->key_pin[3]);
	gpiod_set_pull(gpio_to_desc(pc->key_pin[3]), GPIOD_PULL_DOWN);
	gpio_direction_input(pc->key_pin[0]);
	gpiod_set_pull(gpio_to_desc(pc->key_pin[0]), GPIOD_PULL_DOWN);
	return ret;
}

static int power_ctl_remove(struct platform_device *pdev)
{
	kfree(pc);
	return 0;
}



static const struct of_device_id key_dt_match[] = {
	{.compatible = "amlogic, power_ctl",},
	{},
};

static struct platform_driver kp_driver = {
	.probe      = power_ctl_probe,
	.remove     = power_ctl_remove,
	.driver     = {
		.name   = DRIVE_NAME,
		.of_match_table = key_dt_match,
	},
};

static int __init power_ctl_init(void)
{
	return platform_driver_register(&kp_driver);
}

static void __exit power_ctl_exit(void)
{
	platform_driver_unregister(&kp_driver);
}



late_initcall(power_ctl_init);
module_exit(power_ctl_exit);
MODULE_AUTHOR("Amlogic");
MODULE_DESCRIPTION("ADC Keypad Driver");
MODULE_LICENSE("GPL");
