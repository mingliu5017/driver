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

/* register numbers */
#define INPUT_PORT0	    	   0x00
#define INPUT_PORT1   		   0x01
#define OUTPUT_PORT0     	   0x02
#define OUTPUT_PORT1		   0x03
#define CONFIG_PORT0		   0x04
#define CONFIG_PORT1		   0x05
#define INT_PORT0		       0x06
#define INT_PORT1		       0x07
#define AW95XX_CHIP_ID 		   0x10
#define CHIP_CTL 		       0x11
#define LED_MODE_SWITCH_P0     0x12
#define LED_MODE_SWITCH_P1     0x13
#define LED_BRIGHTNESS(channel)	(0x1f + channel)
#define SW_RESET		        0x7F

#define AW9523B_MAX_LEDS        16

struct aw95xx_chipdef {
	int num_leds;
};

/*
 * regmap is used as a cache of chip's register space,
 * to avoid reading back brightness values from chip,
 * which is known to hang.
 */
struct aw95xx_chip {
	const struct aw95xx_chipdef     *cdef;
	struct i2c_client               *client;
	struct regmap                   *regmap;
	struct mutex                    lock;

	struct aw95xx_led {
		struct aw95xx_chip  *chip;
		struct led_classdev     cdev;
		bool                    configured;
		u8                      channels;
	} leds[AW9523B_MAX_LEDS];
};

static const struct reg_default aw95xx_reg_defaults[] = {
	{ INPUT_PORT0,  0xff},
	{ INPUT_PORT1,  0xff},
	{ OUTPUT_PORT0, 0xff},
	{ OUTPUT_PORT1, 0xff},
	{ CONFIG_PORT0, 0x00},
	{ CONFIG_PORT1, 0x00},
	{ INT_PORT0, 0x00},
	{ INT_PORT1, 0x00},
	{ AW95XX_CHIP_ID, 0x23},
	{ CHIP_CTL, 0x00},
	{ LED_MODE_SWITCH_P0, 0xFF},
	{ LED_MODE_SWITCH_P1, 0xFF},
	{ LED_BRIGHTNESS(0), 0x00},
	{ LED_BRIGHTNESS(1), 0x00},
	{ LED_BRIGHTNESS(2), 0x00},
	{ LED_BRIGHTNESS(3), 0x00},
	{ LED_BRIGHTNESS(4), 0x00},
	{ LED_BRIGHTNESS(5), 0x00},
	{ LED_BRIGHTNESS(6), 0x00},
	{ LED_BRIGHTNESS(7), 0x00},
	{ LED_BRIGHTNESS(8), 0x00},
	{ LED_BRIGHTNESS(9), 0x00},
	{ LED_BRIGHTNESS(10), 0x00},
	{ LED_BRIGHTNESS(11), 0x00},
	{ LED_BRIGHTNESS(12), 0x00},
	{ LED_BRIGHTNESS(13), 0x00},
	{ LED_BRIGHTNESS(14), 0x00},
	{ LED_BRIGHTNESS(15), 0x00},
	{ LED_BRIGHTNESS(16), 0x00},
    { SW_RESET, 0x00},
};

static struct regmap_config aw95xx_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = SW_RESET,
	.cache_type = REGCACHE_NONE,
	.reg_defaults = aw95xx_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(aw95xx_reg_defaults),
};

static int aw95xx_brightness_set(struct led_classdev *cdev,
				     enum led_brightness brightness)
{
	struct aw95xx_led *led = container_of(cdev, struct aw95xx_led,
						  cdev);
	struct aw95xx_chip *aw95 = led->chip;
	int ret;

	//dev_err(&aw95->client->dev, "%s reg 0x%x = %d \n", __func__, LED_BRIGHTNESS(led->channels), brightness);

	mutex_lock(&aw95->lock);

	/* update OUT_COLOR  register */
	ret = regmap_write(aw95->regmap, LED_BRIGHTNESS(led->channels), brightness);
	if (ret < 0)
		goto out;

out:
	mutex_unlock(&aw95->lock);

	return ret;
}

static int aw95xx_parse_child_dt(const struct device *dev,
				     const struct device_node *child,
				     struct aw95xx_led *led)
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

 static const struct aw95xx_chipdef aw9523b_cdef = {
	 .num_leds = 16,
 };

static const struct of_device_id of_aw95xx_match[] = {
	{ .compatible = "awinic,aw9523b", .data = &aw9523b_cdef, },
	{ }
};
MODULE_DEVICE_TABLE(of, of_aw95xx_match);

static int aw95xx_parse_dt(struct device *dev,
			       struct aw95xx_chip *aw95)
{
	struct device_node *np = dev->of_node, *child;
	const struct of_device_id *of_dev_id;
	int count;
	int ret;

	if (!np)
		return -ENODEV;

	of_dev_id = of_match_device(of_aw95xx_match, dev);
	if (!of_dev_id) {
		dev_err(dev, "Failed to match device with supported chips\n");
		return -EINVAL;
	}

	aw95->cdef = of_dev_id->data;

	count = of_get_child_count(np);

	dev_err(dev, "probe %s with %d leds defined in DTS\n",
		of_dev_id->compatible, count);

	if (!count || count > aw95->cdef->num_leds) {
		dev_err(dev, "Number of leds defined must be between 1 and %u\n",
			aw95->cdef->num_leds);
		return -ENODEV;
	}

	for_each_child_of_node(np, child) {
		struct aw95xx_led *led;
		u32 reg;

		ret = of_property_read_u32(child, "reg_offset", &reg);
		if (ret) {
			dev_err(dev, "Failed to read led 'reg' property\n");
			goto put_child_node;
		}

		if (reg < 1 || reg > aw95->cdef->num_leds) {
			dev_err(dev, "invalid led reg %u\n", reg);
			ret = -EINVAL;
			goto put_child_node;
		}

		led = &aw95->leds[reg-1];

		if (led->configured) {
			dev_err(dev, "led %u is already configured\n", reg);
			ret = -EINVAL;
			goto put_child_node;
		}

		ret = aw95xx_parse_child_dt(dev, child, led);
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

static int aw95xx_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
	struct aw95xx_chip *aw95;
	int err,chip_id;
	int i = 0;

	aw95 = devm_kzalloc(&client->dev, sizeof(*aw95), GFP_KERNEL);
	if (!aw95)
		return -ENOMEM;

	mutex_init(&aw95->lock);

	err = aw95xx_parse_dt(&client->dev, aw95);
	if (err)
		goto free_mutex;

	aw95->client = client;
	aw95->regmap = devm_regmap_init_i2c(client, &aw95xx_regmap_config);
	if (IS_ERR(aw95->regmap)) {
		dev_err(&client->dev, "failed to allocate register map.\n");
		err = PTR_ERR(aw95->regmap);
		goto free_mutex;
	}

	i2c_set_clientdata(client, aw95);

	err = regmap_read(aw95->regmap, AW95XX_CHIP_ID, &chip_id);
	if (err < 0) {
		dev_err(&client->dev, "read AW9523B chip ID faild.err = %d\n",err);
		err = -EIO; /* does not answer */
		goto free_mutex;
	}else{
         dev_err(&client->dev, "AW9523B chip ID = 0x%x.\n",chip_id);
    }

	err = regmap_write(aw95->regmap, LED_MODE_SWITCH_P0, 0x00);
	if (err < 0) {
		dev_err(&client->dev, "no response from chip write: err = %d\n",err);
		err = -EIO; /* does not answer */
		goto free_mutex;
	}

	err = regmap_write(aw95->regmap, LED_MODE_SWITCH_P1, 0x00);
	if (err < 0) {
		dev_err(&client->dev, "no response from chip write: err = %d\n",err);
		err = -EIO; /* does not answer */
		goto free_mutex;
	}
#if 0
	for(i = 0; i < 16; i++) {
		err = regmap_write(aw95->regmap, LED_BRIGHTNESS(i), 10);
		if (err < 0)
			goto free_mutex;
	}
#endif
	for (i = 0; i < aw95->cdef->num_leds; i++) {
		struct aw95xx_led *led = &aw95->leds[i];

		if (!led->configured)
			continue;

		led->chip = aw95;
		led->cdev.brightness_set_blocking = aw95xx_brightness_set;

		err = devm_led_classdev_register(&client->dev, &led->cdev);
		if (err < 0)
			goto free_mutex;
	}

	return 0;

free_mutex:
	mutex_destroy(&aw95->lock);
	return err;
}

static int aw95xx_remove(struct i2c_client *client)
{
	struct aw95xx_chip *aw95 = i2c_get_clientdata(client);

	mutex_destroy(&aw95->lock);
	return 0;
}

/*
 * i2c-core (and modalias) requires that id_table be properly filled,
 * even though it is not used for DeviceTree based instantiation.
 */
static const struct i2c_device_id aw95xx_id[] = {
	{ "aw9523b" },
	{},
};
MODULE_DEVICE_TABLE(i2c, aw95xx_id);

static struct i2c_driver aw95xx_driver = {
	.driver   = {
		.name           = "aw95xx",
		.of_match_table = of_match_ptr(of_aw95xx_match),
	},
	.probe    = aw95xx_probe,
	.remove   = aw95xx_remove,
	.id_table = aw95xx_id,
};

module_i2c_driver(aw95xx_driver);

MODULE_AUTHOR("Liu Ming <ming.liu@amlogic.com>");
MODULE_DESCRIPTION("aw95xx LED driver");
MODULE_LICENSE("GPL v2");
