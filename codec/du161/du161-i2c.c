/*
 * Driver for the DU161 CODEC
 *
 * Author:	Liu Ming <ming.liu@3nod.com>
 *		Copyright 2018 Linaro Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>

#include "du161.h"

static int du161_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct regmap *regmap;

	regmap = devm_regmap_init_i2c(i2c, &du161_regmap);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	return du161_probe(&i2c->dev, regmap);
}

static int du161_i2c_remove(struct i2c_client *i2c)
{
	du161_remove(&i2c->dev);
	return 0;
}

static const struct i2c_device_id du161_i2c_id[] = {
	{ "du161", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, du161_i2c_id);

static const struct of_device_id du161_of_match[] = {
	{ .compatible = "MVSILICON,du161", },
    { /* senitel */ }
};
MODULE_DEVICE_TABLE(of, du161_of_match);

static struct i2c_driver du161_i2c_driver = {
	.probe 		= du161_i2c_probe,
	.remove 	= du161_i2c_remove,
	.id_table	= du161_i2c_id,
	.driver		= {
		.name	= "du161",
		.of_match_table = du161_of_match,
		.owner = THIS_MODULE,
	},
};

module_i2c_driver(du161_i2c_driver);

MODULE_DESCRIPTION("ASoC DU161 codec driver - I2C");
MODULE_AUTHOR("Liu Ming <ming.liu@3nod.com>");
MODULE_LICENSE("GPL v2");
