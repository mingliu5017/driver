/*
 * Driver for the TAS5766M Audio Amplifier
 *
 * Author: Andy Liu <andy-liu@ti.com>
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

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/delay.h>

#include <sound/soc.h>
#include <sound/pcm.h>
#include <sound/initval.h>

#include "tas5766m.h"

#define TAS5766M_DRV_NAME    "tas5766m"

#define TAS5766M_RATES	     (SNDRV_PCM_RATE_48000)
#define TAS5766M_FORMATS     (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
			                  SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

#define TAS5766M_REG_00      (0x00)
#define TAS5766M_REG_01      (0x01)
#define TAS5766M_REG_03      (0x03)
#define TAS5766M_REG_08      (0x08)
#define TAS5766M_REG_09      (0x09)
#define TAS5766M_REG_0A      (0x0A)
#define TAS5766M_REG_0B      (0x0B)

#define TAS5766M_PAGE_00     (0x00)
#define TAS5766M_PAGE_2C     (0x2C)

#define TAS5766M_VOLUME_MAX  (232)
#define TAS5766M_VOLUME_MIN  (0)

struct tas5766m_priv {
	struct regmap *regmap;

	int vol;
};

const struct regmap_config tas5766m_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.cache_type = REGCACHE_RBTREE,
};

static int tas5766m_vol_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)
{
	uinfo->type   = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->access = SNDRV_CTL_ELEM_ACCESS_TLV_READ | SNDRV_CTL_ELEM_ACCESS_READWRITE;
	uinfo->count  = 1;
	
	uinfo->value.integer.min  = TAS5766M_VOLUME_MIN;
	uinfo->value.integer.max  = TAS5766M_VOLUME_MAX;
	uinfo->value.integer.step = 1;

	return 0;
}

static int tas5766m_vol_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5766m_priv *tas5766m = snd_soc_codec_get_drvdata(codec);
			
	ucontrol->value.integer.value[0] = tas5766m->vol;

	return 0;
}

static inline int get_volume_index(int vol)
{
   int index;

   //input  : 232 -- 0
   //output : 0   -- 232
   index = TAS5766M_VOLUME_MAX - vol;
   if (index < TAS5766M_VOLUME_MIN) index = TAS5766M_VOLUME_MIN;
   if (index > TAS5766M_VOLUME_MAX) index = TAS5766M_VOLUME_MAX;
	
   return index;
}

static void tas5766m_set_volume(struct snd_soc_codec *codec, int vol)
{
    int index;
	uint32_t volume_hex;
	uint8_t byte3;
	uint8_t byte2;
	uint8_t byte1;

	index = get_volume_index(vol);
	volume_hex = tas5766m_volume[index];
	byte3 = ((volume_hex >> 16) & 0xFF);
	byte2 = ((volume_hex >> 8)	& 0xFF);
	byte1 = ((volume_hex >> 0)	& 0xFF);
	
	//w 98 00 2c
	snd_soc_write(codec, TAS5766M_REG_00, TAS5766M_PAGE_2C);
	//w 98 08 byte3
	snd_soc_write(codec, TAS5766M_REG_08, byte3);
	//w 98 09 byte2
	snd_soc_write(codec, TAS5766M_REG_09, byte2);
    //w 98 0a byte1 
	snd_soc_write(codec, TAS5766M_REG_0A, byte1);
	//w 98 0b 00 
	snd_soc_write(codec, TAS5766M_REG_0B, 0x00);
	
	//w 98 01 05
	snd_soc_write(codec, TAS5766M_REG_01, 0x05);
    msleep(3);
	
	//w 98 08 byte3
	snd_soc_write(codec, TAS5766M_REG_08, byte3);
	//w 98 09 byte2
	snd_soc_write(codec, TAS5766M_REG_09, byte2);
    //w 98 0a byte1 
	snd_soc_write(codec, TAS5766M_REG_0A, byte1);
	//w 98 0b 00 
	snd_soc_write(codec, TAS5766M_REG_0B, 0x00);
}

static int tas5766m_vol_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5766m_priv *tas5766m = snd_soc_codec_get_drvdata(codec);	
	int vol;
	
	vol = ucontrol->value.integer.value[0];

	tas5766m->vol = vol;
	tas5766m_set_volume(codec, vol);

	return 0;
}

static const struct snd_kcontrol_new tas5766m_vol_control = 
{	
    .iface = SNDRV_CTL_ELEM_IFACE_MIXER, 
    .name  = "TAS5766M Software Volume", 
	.info  = tas5766m_vol_info, 
	.get   = tas5766m_vol_get,
	.put   = tas5766m_vol_put, 
};

static int tas5766m_snd_probe(struct snd_soc_codec *codec)
{
    int ret;
		
	ret = snd_soc_add_codec_controls(codec, &tas5766m_vol_control, 1);
		
    return ret;
}

static struct snd_soc_codec_driver soc_codec_tas5766m = {
	.probe = tas5766m_snd_probe,
};

static int tas5766m_mute(struct snd_soc_dai *dai, int mute)
{
	u8 reg3_value = 0;
	struct snd_soc_codec *codec = dai->codec;

	if (mute)
		reg3_value = 0x11;
	
	snd_soc_write(codec, TAS5766M_REG_00, 0x00);
	snd_soc_write(codec, TAS5766M_REG_03, reg3_value);

	return 0;
}

static int tas5766m_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
//	struct snd_soc_codec *codec = dai->codec;
//	struct tas5766m_priv *tas5766m = snd_soc_codec_get_drvdata(codec);

	return 0;
}

static const struct snd_soc_dai_ops tas5766m_dai_ops = {
	.digital_mute = tas5766m_mute,
	.prepare      = tas5766m_prepare,
};

static struct snd_soc_dai_driver tas5766m_dai = {
	.name		= "tas5766m-amplifier",
	.playback 	= {
		.stream_name	= "Playback",
		.channels_min	= 2,
		.channels_max	= 2,
		.rates		= TAS5766M_RATES,
		.formats	= TAS5766M_FORMATS,
	},
	.ops = &tas5766m_dai_ops,
};

static int tas5766m_probe(struct device *dev, struct regmap *regmap)
{
	struct tas5766m_priv *tas5766m;
	int ret;
		
	tas5766m = devm_kzalloc(dev, sizeof(struct tas5766m_priv), GFP_KERNEL);
	if (!tas5766m)
		return -ENOMEM;

	dev_set_drvdata(dev, tas5766m);
	tas5766m->regmap = regmap;
    tas5766m->vol = 150;    //-35dB, 65%

	ret = regmap_register_patch(regmap, tas5766m_init_sequence1, ARRAY_SIZE(tas5766m_init_sequence1));
	if (ret != 0)
	{
		dev_err(dev, "Failed to initialize TAS5766M: %d\n",ret);
		//goto err;
	}

	ret = regmap_register_patch(regmap, tas5766m_init_sequence2, ARRAY_SIZE(tas5766m_init_sequence2));
	if (ret != 0)
	{
		dev_err(dev, "Failed to initialize TAS5766M: %d\n",ret);
		//goto err;
	}

	ret = regmap_register_patch(regmap, tas5766m_init_sequence3, ARRAY_SIZE(tas5766m_init_sequence3));
	if (ret != 0)
	{
		dev_err(dev, "Failed to initialize TAS5766M: %d\n",ret);
		//goto err;
	}

	ret = regmap_register_patch(regmap, tas5766m_init_sequence4, ARRAY_SIZE(tas5766m_init_sequence4));
	if (ret != 0)
	{
		dev_err(dev, "Failed to initialize TAS5766M: %d\n",ret);
		//goto err;
	}

	ret = snd_soc_register_codec(dev, 
	                             &soc_codec_tas5766m,
			                     &tas5766m_dai, 
								 1);
	if (ret != 0) 
	{
		dev_err(dev, "Failed to register CODEC: %d\n", ret);
		goto err;
	}

	return 0;
	
err:
	return ret;
}

static int tas5766m_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{	
	struct regmap *regmap;
	struct regmap_config config = tas5766m_regmap;
	
	regmap = devm_regmap_init_i2c(i2c, &config);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	return tas5766m_probe(&i2c->dev, regmap);
}

static int tas5766m_remove(struct device *dev)
{
	snd_soc_unregister_codec(dev);
	
	return 0;
}

static int tas5766m_i2c_remove(struct i2c_client *i2c)
{
	tas5766m_remove(&i2c->dev);
	
	return 0;
}

static const struct i2c_device_id tas5766m_i2c_id[] = {
	{ "tas5766m", },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tas5766m_i2c_id);

#ifdef CONFIG_OF
static const struct of_device_id tas5766m_of_match[] = {
	{ .compatible = "ti,tas5766m", },
	{ }
};
MODULE_DEVICE_TABLE(of, tas5766m_of_match);
#endif

static struct i2c_driver tas5766m_i2c_driver = {
	.probe 		= tas5766m_i2c_probe,
	.remove 	= tas5766m_i2c_remove,
	.id_table	= tas5766m_i2c_id,
	.driver		= {
		.name	= TAS5766M_DRV_NAME,
		.of_match_table = tas5766m_of_match,
	},
};

module_i2c_driver(tas5766m_i2c_driver);

MODULE_AUTHOR("Andy Liu <andy-liu@ti.com>");
MODULE_DESCRIPTION("TAS5766M Audio Amplifier Driver - SmartAmp");
MODULE_LICENSE("GPL");
