/*
 * Author: Liu Ming <ming.liu@amlogic.com>
 *
 * This file is subject to the terms and conditions of version 2 of
 * the GNU General Public License.  See the file COPYING in the main
 * directory of this archive for more details.
 *
 */
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/regmap.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/slab.h>
#include "../../gpio/gpiolib.h"

/* register numbers */
#define DEVICE_CONFIG0	    	0x00
#define DEVICE_CONFIG1   		0x01
#define LED_CONFIG0     		0x02
#define BANK_BRIGHTNESS		    0x03
#define BANK_A_COLOR		    0x04
#define BANK_B_COLOR		    0x05
#define BANK_C_COLOR		    0x06
#define LED_BRIGHTNESS(channel)	(0x06 + channel)
#define OUT_COLOR(channel)	    (0x0E + channel)

#define RESET		            0x27

#define lp50xx_MAX_LEDS         24

struct lp50xx_chipdef {
	int num_leds;
};

/*
 * regmap is used as a cache of chip's register space,
 * to avoid reading back brightness values from chip,
 * which is known to hang.
 */
struct lp50xx_chip {
	const struct lp50xx_chipdef *cdef;
	struct i2c_client               *client;
	struct regmap                   *regmap;
	struct mutex                    lock;

	struct lp50xx_led {
		struct lp50xx_chip  *chip;
		struct led_classdev     cdev;
		bool                    configured;
		u8                      channels;
	} leds[lp50xx_MAX_LEDS];
};

static const struct reg_default lp50xx_reg_defaults[] = {
	{ DEVICE_CONFIG0, 0x00},
	{ DEVICE_CONFIG1, 0x3C},
	{ LED_CONFIG0, 0x00},
	{ BANK_BRIGHTNESS, 0xFF},
	{ BANK_A_COLOR, 0x00},
	{ BANK_B_COLOR, 0x00},
	{ BANK_C_COLOR, 0x00},
	{ LED_BRIGHTNESS(0), 0xFF},
	{ LED_BRIGHTNESS(1), 0xFF},
	{ LED_BRIGHTNESS(2), 0xFF},
	{ LED_BRIGHTNESS(3), 0xFF},
	{ LED_BRIGHTNESS(4), 0xFF},
	{ LED_BRIGHTNESS(5), 0xFF},
	{ LED_BRIGHTNESS(6), 0xFF},
	{ LED_BRIGHTNESS(7), 0xFF},
	{ OUT_COLOR(0), 0x00},
	{ OUT_COLOR(1), 0x00},
	{ OUT_COLOR(2), 0x00},
	{ OUT_COLOR(3), 0x00},
	{ OUT_COLOR(4), 0x00},
	{ OUT_COLOR(5), 0x00},
	{ OUT_COLOR(6), 0x00},
	{ OUT_COLOR(7), 0x00},
	{ OUT_COLOR(8), 0x00},
	{ OUT_COLOR(9), 0x00},
	{ OUT_COLOR(10), 0x00},
	{ OUT_COLOR(11), 0x00},
	{ OUT_COLOR(12), 0x00},
	{ OUT_COLOR(13), 0x00},
	{ OUT_COLOR(14), 0x00},
	{ OUT_COLOR(15), 0x00},
	{ OUT_COLOR(16), 0x00},
	{ OUT_COLOR(17), 0x00},
	{ OUT_COLOR(18), 0x00},
	{ OUT_COLOR(19), 0x00},
	{ OUT_COLOR(20), 0x00},
	{ OUT_COLOR(21), 0x00},
	{ OUT_COLOR(22), 0x00},
	{ OUT_COLOR(23), 0x00},
    { RESET, 0x00},
};

static struct regmap_config lp50xx_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = RESET,
	.cache_type = REGCACHE_NONE,
	.reg_defaults = lp50xx_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(lp50xx_reg_defaults),
};

static int lp50xx_brightness_set(struct led_classdev *cdev,
				     enum led_brightness brightness)
{
	struct lp50xx_led *led = container_of(cdev, struct lp50xx_led,
						  cdev);
	struct lp50xx_chip *lp50 = led->chip;
	int ret;

	dev_dbg(&lp50->client->dev, "%s %d: %d \r\n", __func__, led->channels, brightness);

	mutex_lock(&lp50->lock);

	/* update OUT_COLOR  register */
	ret = regmap_write(lp50->regmap, OUT_COLOR(led->channels), brightness);
	if (ret < 0)
		goto out;

out:
	mutex_unlock(&lp50->lock);

	return ret;
}

static int lp50xx_parse_child_dt(const struct device *dev,
				     const struct device_node *child,
				     struct lp50xx_led *led)
{
	struct led_classdev *cdev = &led->cdev;
	int ret;
	u32 reg;

	if (of_property_read_string(child, "label", &cdev->name))
		cdev->name = child->name;

	ret = of_property_read_u32(child, "reg_offset", &reg);
	led->channels = reg;

	ret = of_property_read_string(child, "linux,default-trigger",
				      &cdev->default_trigger);
	if (ret < 0 && ret != -EINVAL) /* is optional */
		return ret;

	return 0;
}
static const struct lp50xx_chipdef lp5024_cdef = {
	.num_leds = 24,
};
 
static const struct of_device_id of_lp50xx_match[] = {
	{ .compatible = "ti,lp5024", .data = &lp5024_cdef, },
	{ }
};
MODULE_DEVICE_TABLE(of, of_lp50xx_match);

static int lp50xx_parse_dt(struct device *dev,
			       struct lp50xx_chip *lp50)
{
	struct device_node *np = dev->of_node, *child;
	const struct of_device_id *of_dev_id;
	int count;
	int ret;

	if (!np)
		return -ENODEV;

	of_dev_id = of_match_device(of_lp50xx_match, dev);
	if (!of_dev_id) {
		dev_err(dev, "Failed to match device with supported chips\n");
		return -EINVAL;
	}

	lp50->cdef = of_dev_id->data;

	count = of_get_child_count(np);

	dev_err(dev, "probe %s with %d leds defined in DT\n",
		of_dev_id->compatible, count);

	if (!count || count > lp50->cdef->num_leds) {
		dev_err(dev, "Number of leds defined must be between 1 and %u\n",
			lp50->cdef->num_leds);
		return -ENODEV;
	}

	for_each_child_of_node(np, child) {
		struct lp50xx_led *led;
		u32 reg;

		ret = of_property_read_u32(child, "reg_offset", &reg);
		if (ret) {
			dev_err(dev, "Failed to read led 'reg' property\n");
			goto put_child_node;
		}

		if (reg < 1 || reg > lp50->cdef->num_leds) {
			dev_err(dev, "invalid led reg %u\n", reg);
			ret = -EINVAL;
			goto put_child_node;
		}

		led = &lp50->leds[reg-1];

		if (led->configured) {
			dev_err(dev, "led %u is already configured\n", reg);
			ret = -EINVAL;
			goto put_child_node;
		}

		ret = lp50xx_parse_child_dt(dev, child, led);
		if (ret) {
			dev_err(dev, "led %u DT parsing failed\n", reg);
			goto put_child_node;
		}

		led->configured = true;
	}

	return 0;

put_child_node:
	of_node_put(child);
	return ret;
}

static int lp50xx_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
	struct lp50xx_chip *lp50;
	int err;
	int i = 0;

	lp50 = devm_kzalloc(&client->dev, sizeof(*lp50), GFP_KERNEL);
	if (!lp50)
		return -ENOMEM;

	mutex_init(&lp50->lock);

	err = lp50xx_parse_dt(&client->dev, lp50);
	if (err)
		goto free_mutex;

	lp50->client = client;
	lp50->regmap = devm_regmap_init_i2c(client, &lp50xx_regmap_config);
	if (IS_ERR(lp50->regmap)) {
		dev_err(&client->dev, "failed to allocate register map\n");
		err = PTR_ERR(lp50->regmap);
		goto free_mutex;
	}

	i2c_set_clientdata(client, lp50);

	err = regmap_write(lp50->regmap, DEVICE_CONFIG0, 0x40);
	if (err < 0) {
		dev_err(&client->dev, "no response from chip write: err = %d\n",err);
		err = -EIO; /* does not answer */
		goto free_mutex;
	}

	err = regmap_write(lp50->regmap, DEVICE_CONFIG1, 0x3e);
	if (err < 0) {
		dev_err(&client->dev, "no response from chip write: err = %d\n",err);
		err = -EIO; /* does not answer */
		goto free_mutex;
	}

	for (i = 0; i < lp50->cdef->num_leds; i++) {
		struct lp50xx_led *led = &lp50->leds[i];

		if (!led->configured)
			continue;

		led->chip = lp50;
		led->cdev.brightness_set_blocking = lp50xx_brightness_set;

		err = devm_led_classdev_register(&client->dev, &led->cdev);
		if (err < 0)
			goto free_mutex;
	}

	return 0;

free_mutex:
	mutex_destroy(&lp50->lock);
	return err;
}

static int lp50xx_remove(struct i2c_client *client)
{
	struct lp50xx_chip *lp50 = i2c_get_clientdata(client);

	mutex_destroy(&lp50->lock);
	return 0;
}

/*
 * i2c-core (and modalias) requires that id_table be properly filled,
 * even though it is not used for DeviceTree based instantiation.
 */
static const struct i2c_device_id lp50xx_id[] = {
	{ "lp5024" },
	{},
};
MODULE_DEVICE_TABLE(i2c, lp50xx_id);

static struct i2c_driver lp50xx_driver = {
	.driver   = {
		.name           = "lp50xx",
		.of_match_table = of_match_ptr(of_lp50xx_match),
	},
	.probe    = lp50xx_probe,
	.remove   = lp50xx_remove,
	.id_table = lp50xx_id,
};

module_i2c_driver(lp50xx_driver);

MODULE_AUTHOR("Liu Ming <ming.liu@amlogic.com>");
MODULE_DESCRIPTION("LP50xx LED driver");
MODULE_LICENSE("GPL v2");
