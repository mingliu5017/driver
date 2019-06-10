/*
 * sound/soc/codecs/amlogic/tas5751m.c
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
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/tas57xx.h>
#include <linux/amlogic/aml_gpio_consumer.h>

#include "tas5751m.h"

#define DEV_NAME	"tas5751m"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static void tas5751m_early_suspend(struct early_suspend *h);
static void tas5751m_late_resume(struct early_suspend *h);
#endif

#define tas5751m_RATES (SNDRV_PCM_RATE_8000 | \
		       SNDRV_PCM_RATE_11025 | \
		       SNDRV_PCM_RATE_16000 | \
		       SNDRV_PCM_RATE_22050 | \
		       SNDRV_PCM_RATE_32000 | \
		       SNDRV_PCM_RATE_44100 | \
		       SNDRV_PCM_RATE_48000)

#define tas5751m_FORMATS \
	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE | \
	 SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE | \
	 SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE | \
	 SNDRV_PCM_FMTBIT_S32_LE)

#define tas5751m_REG_MUTE               (0x06)
#define tas5751m_REG_MASTER_VOL      	(0x07)
#define tas5751m_REG_CHANNEL_1_VOL      (0x08)
#define tas5751m_REG_CHANNEL_2_VOL      (0x09)

#define DEFAULT_VOLUME  (160)

enum BITSIZE_MODE {
	BITSIZE_MODE_16BITS = 0,
	BITSIZE_MODE_20BITS = 1,
	BITSIZE_MODE_24BITS = 2,
	BITSIZE_MODE_32BITS = 3,
};

/* codec private data */
struct tas5751m_priv {
	struct i2c_client *i2c;
	struct regmap *regmap;
	struct snd_soc_codec *codec;
	struct tas57xx_platform_data *pdata;
	struct work_struct work;

	char channel_type[32];

	/*Platform provided EQ configuration */
	int num_eq_conf_texts;
	const char **eq_conf_texts;
	int eq_cfg;
	struct soc_enum eq_conf_enum;
	
	unsigned char Ch1_vol;
	unsigned char Ch2_vol;
	unsigned char master_vol;
	unsigned char mute; //0:unmuted  1:muted
	
	unsigned int mclk;
	unsigned int EQ_enum_value;
	unsigned int DRC_enum_value;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};


static int tas5751m_master_vol_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type   = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->access = SNDRV_CTL_ELEM_ACCESS_TLV_READ
			| SNDRV_CTL_ELEM_ACCESS_READWRITE;
	uinfo->count  = 1;

	uinfo->value.integer.min  = 0;
	uinfo->value.integer.max  = 0xff;
	uinfo->value.integer.step = 1;

	return 0;
}

static int tas5751m_ch1_vol_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type   = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->access = SNDRV_CTL_ELEM_ACCESS_TLV_READ
			| SNDRV_CTL_ELEM_ACCESS_READWRITE;
	uinfo->count  = 1;

	uinfo->value.integer.min  = 0;
	uinfo->value.integer.max  = 0xff;
	uinfo->value.integer.step = 1;

	return 0;
}

static int tas5751m_ch2_vol_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type   = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->access = SNDRV_CTL_ELEM_ACCESS_TLV_READ
			| SNDRV_CTL_ELEM_ACCESS_READWRITE;
	uinfo->count  = 1;

	uinfo->value.integer.min  = 0;
	uinfo->value.integer.max  = 0xff;
	uinfo->value.integer.step = 1;

	return 0;
}

static int tas5751m_mute_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type   = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->access = SNDRV_CTL_ELEM_ACCESS_TLV_READ
			| SNDRV_CTL_ELEM_ACCESS_READWRITE;
	uinfo->count  = 1;

	uinfo->value.integer.min  = 0;
	uinfo->value.integer.max  = 1;
	uinfo->value.integer.step = 1;

	return 0;
}				

static int tas5751m_master_vol_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5751m_priv *tas5751m = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = tas5751m->master_vol;

	return 0;
}


static int tas5751m_ch1_vol_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5751m_priv *tas5751m = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = tas5751m->Ch1_vol;

	return 0;
}

static int tas5751m_ch2_vol_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5751m_priv *tas5751m = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = tas5751m->Ch2_vol;

	return 0;
}

static int tas5751m_mute_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5751m_priv *tas5751m = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = tas5751m->mute;

	return 0;
}


static void tas5751m_set_volume(struct snd_soc_codec *codec,
				int value, int ch_reg)
{
	unsigned char buf[3] = {0};
	int buf_size = ARRAY_SIZE(buf);
	struct tas5751m_priv *tas5751m = snd_soc_codec_get_drvdata(codec);
	unsigned int vol_set = 0;

	buf[0] = ch_reg;
	
	if (value < 0)
		value = 0;
	if (value > 255)
		value = 255;

	if(value == 0){
		vol_set = 0x03FF;
	} else {
		value = 255 - value;
		vol_set = value;
	}

	buf[1] = (vol_set >> 8) & 0xFF;
	buf[2] = (vol_set) & 0xFF;
	
	if(buf_size != i2c_master_send(tas5751m->i2c, buf, buf_size)){
		pr_err("%s %d write volume reg:0x%x error!\n", __func__, __LINE__, ch_reg);
	}
}

static void tas5751m_set_mute(struct snd_soc_codec *codec, int value)
{
	struct tas5751m_priv *tas5751m = snd_soc_codec_get_drvdata(codec);
	unsigned char buf[2];
	int buf_size = ARRAY_SIZE(buf);

	buf[0] = tas5751m_REG_MUTE;
	buf[1] = (value == 1) ? 0x07 : 0x00;
	
	if(buf_size != i2c_master_send(tas5751m->i2c, buf, buf_size)){
		pr_err("%s %d write mute reg:0x%x error!\n", __func__, __LINE__, tas5751m_REG_MUTE);
	}
}


static int tas5751m_master_vol_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5751m_priv *tas5751m = snd_soc_codec_get_drvdata(codec);
	int value;

	value = ucontrol->value.integer.value[0];
	tas5751m->master_vol = value;

	pr_info("%s %d master_vol: %d\n", __func__, __LINE__, tas5751m->master_vol);
	tas5751m_set_volume(codec, value, tas5751m_REG_MASTER_VOL);

	return 0;
}
				

static int tas5751m_ch1_vol_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5751m_priv *tas5751m = snd_soc_codec_get_drvdata(codec);
 	int value = ucontrol->value.integer.value[0];
	
	tas5751m->Ch1_vol = value;
	pr_info("%s %d Ch1_vol: %d\n", __func__, __LINE__, tas5751m->Ch1_vol);
	tas5751m_set_volume(codec, value, tas5751m_REG_CHANNEL_1_VOL);

	return 0;
}

static int tas5751m_ch2_vol_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5751m_priv *tas5751m = snd_soc_codec_get_drvdata(codec);
	int value = ucontrol->value.integer.value[0];
	
	tas5751m->Ch2_vol = value;
	pr_info("%s %d Ch2_vol: %d\n", __func__, __LINE__, tas5751m->Ch2_vol);
	tas5751m_set_volume(codec, value, tas5751m_REG_CHANNEL_2_VOL);

	return 0;
}

static int tas5751m_mute_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct tas5751m_priv *tas5751m = snd_soc_codec_get_drvdata(codec);
	int value = ucontrol->value.integer.value[0];
	
	tas5751m->mute = value & 0x1;
	pr_info("%s %d mute: %d\n", __func__, __LINE__, tas5751m->mute);
	tas5751m_set_mute(codec, value);
	return 0;
}				


static const struct snd_kcontrol_new tas5751m_snd_controls[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name  = "Master Volume",
		.info  = tas5751m_master_vol_info,
		.get   = tas5751m_master_vol_get,
		.put   = tas5751m_master_vol_put,
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name  = "Ch1 Volume",
		.info  = tas5751m_ch1_vol_info,
		.get   = tas5751m_ch1_vol_get,
		.put   = tas5751m_ch1_vol_put,
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name  = "Ch2 Volume",
		.info  = tas5751m_ch2_vol_info,
		.get   = tas5751m_ch2_vol_get,
		.put   = tas5751m_ch2_vol_put,
	},
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name  = "Mute Control",
		.info  = tas5751m_mute_info,
		.get   = tas5751m_mute_get,
		.put   = tas5751m_mute_put,
	}	
};

static int tas5751m_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int tas5751m_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		return 0;//-EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		break;
	default:
		return 0;//-EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_NB_IF:
		break;
	default:
		return 0;//-EINVAL;
	}

	return 0;
}

static int tas5751m_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	unsigned int rate;

	rate = params_rate(params);

	pr_debug("rate: %u\n", rate);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S24_BE:
		pr_debug("24bit\n");
	/* fall through */
	case SNDRV_PCM_FORMAT_S32_LE:
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S20_3BE:
		pr_debug("20bit\n");

		break;
	case SNDRV_PCM_FORMAT_S16_LE:
	case SNDRV_PCM_FORMAT_S16_BE:
		pr_debug("16bit\n");

		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int tas5751m_set_bias_level(struct snd_soc_codec *codec,
				enum snd_soc_bias_level level)
{
	pr_debug("level = %d\n", level);

	switch (level) {
	case SND_SOC_BIAS_ON:
		break;

	case SND_SOC_BIAS_PREPARE:
		/* Full power on */
		break;

	case SND_SOC_BIAS_STANDBY:
		break;

	case SND_SOC_BIAS_OFF:
		/* The chip runs through the power down sequence for us. */
		break;
	}
	codec->component.dapm.bias_level = level;

	return 0;
}

static const struct snd_soc_dai_ops tas5751m_dai_ops = {
	.hw_params = tas5751m_hw_params,
	.set_sysclk = tas5751m_set_dai_sysclk,
	.set_fmt = tas5751m_set_dai_fmt,
};

static struct snd_soc_dai_driver tas5751m_dai = {
	.name = DEV_NAME,
	.playback = {
		.stream_name = "HIFI Playback",
		.channels_min = 2,
		.channels_max = 8,
		.rates = tas5751m_RATES,
		.formats = tas5751m_FORMATS,
	},
	.ops = &tas5751m_dai_ops,
};


static int reset_tas5751m_GPIO(struct snd_soc_codec *codec)
{
	struct tas5751m_priv *tas5751m = snd_soc_codec_get_drvdata(codec);
	struct tas57xx_platform_data *pdata = tas5751m->pdata;
	int ret = 0;

	if (pdata->reset_pin < 0)
		return 0;

	ret = devm_gpio_request_one(codec->dev, pdata->reset_pin,
				GPIOF_OUT_INIT_LOW, "tas5751m-reset-pin");

	if (ret < 0) {
		pr_err("failed!!! devm_gpio_request_one = %d!\n", ret);
		return -1;
	}

	gpio_direction_output(pdata->reset_pin, GPIOF_OUT_INIT_LOW);
	udelay(1000);
	gpio_direction_output(pdata->reset_pin, GPIOF_OUT_INIT_HIGH);
	udelay(1000);
	gpio_direction_output(pdata->reset_pin, GPIOF_OUT_INIT_LOW);
	msleep(20);
	pr_err("%s %d gpio reset done!\n", __func__, __LINE__);
	return 0;
}


static void tas5751m_reg_init(struct tas5751m_priv *tas5751m)
{
	struct snd_soc_codec *codec;
	struct i2c_client *i2c;
	int value_count = 0;
	int value_offset = 0;
	unsigned char *preg_values = NULL;

	codec = tas5751m->codec;

	reset_tas5751m_GPIO(codec);

	//init register
	tas5751m = snd_soc_codec_get_drvdata(codec);
	i2c = tas5751m->i2c;

	pr_info("%s %d slave_addr:0x%x channel_type:%s\n", __func__, __LINE__, \
		i2c->addr, tas5751m->channel_type);
	
	if( 0 == strcmp(tas5751m->channel_type, "LFE") ){
		preg_values = &reg_default_lfe[0];
		value_count = ARRAY_SIZE(reg_default_lfe);
		pr_info("%s %d reg_default_lfe \n", __func__, __LINE__);
	} else {
		preg_values = &reg_default[0];
		value_count = ARRAY_SIZE(reg_default);
		pr_info("%s %d reg_default \n", __func__, __LINE__);
	}

	while(value_offset < value_count) {
		unsigned char *pdata = preg_values + value_offset;
		int w = *pdata + 1;
		if(w > 1) {
			int cur_w = i2c_master_send(i2c, pdata + 1, w);
			if(cur_w != w ){
				pr_err("%s %d i2c_master_send error!  cur_w:%d w:%d value_offset:%d\n",\
					__func__, __LINE__, cur_w, w, value_offset);
				break;
			}

			value_offset += 1 + w;
		} else {
			break;
		}
	}

	tas5751m_set_mute(codec, 0);
	tas5751m->mute = 0;
	tas5751m_set_volume(codec, DEFAULT_VOLUME, tas5751m_REG_MASTER_VOL);
	tas5751m->master_vol = DEFAULT_VOLUME;
	tas5751m_set_volume(codec, DEFAULT_VOLUME, tas5751m_REG_CHANNEL_1_VOL);
	tas5751m->Ch1_vol = DEFAULT_VOLUME;
	tas5751m_set_volume(codec, DEFAULT_VOLUME, tas5751m_REG_CHANNEL_2_VOL);
	tas5751m->Ch2_vol = DEFAULT_VOLUME;
}

static int tas5751m_probe(struct snd_soc_codec *codec)
{
	struct tas5751m_priv *tas5751m;

#ifdef CONFIG_HAS_EARLYSUSPEND
	tas5751m->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	tas5751m->early_suspend.suspend = tas5751m_early_suspend;
	tas5751m->early_suspend.resume = tas5751m_late_resume;
	tas5751m->early_suspend.param = codec;
	register_early_suspend(&(tas5751m->early_suspend));
#endif

	tas5751m = snd_soc_codec_get_drvdata(codec);
	tas5751m->codec = codec;

	tas5751m_reg_init(tas5751m);


	//INIT_WORK(&tas5751m->work, tas5751m_reg_init);
	//schedule_work(&tas5751m->work);

	return 0;
}

static int tas5751m_remove(struct snd_soc_codec *codec)
{
	struct tas5751m_priv *tas5751m;

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct tas5751m_priv *tas5751m = snd_soc_codec_get_drvdata(codec);

	unregister_early_suspend(&(tas5751m->early_suspend));
#endif
	tas5751m = snd_soc_codec_get_drvdata(codec);

	//cancel_work_sync(&tas5751m->work);

	return 0;
}

#ifdef CONFIG_PM
static int tas5751m_suspend(struct snd_soc_codec *codec)
{
	struct tas57xx_platform_data *pdata = dev_get_platdata(codec->dev);

	dev_info(codec->dev, "tas5751m_suspend!\n");

	if (pdata && pdata->suspend_func)
		pdata->suspend_func();

	return 0;
}

static int tas5751m_resume(struct snd_soc_codec *codec)
{
	struct tas57xx_platform_data *pdata = dev_get_platdata(codec->dev);
	struct tas5751m_priv *tas5751m;

	dev_info(codec->dev, "tas5751m_resume!\n");

	if (pdata && pdata->resume_func)
		pdata->resume_func();

	tas5751m = snd_soc_codec_get_drvdata(codec);
	tas5751m->codec = codec;

	tas5751m_reg_init(tas5751m);
	
	//INIT_WORK(&tas5751m->work, tas5751m_reg_init);
	//schedule_work(&tas5751m->work);


	return 0;
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void tas5751m_early_suspend(struct early_suspend *h)
{
}

static void tas5751m_late_resume(struct early_suspend *h)
{
}
#endif

static const struct snd_soc_dapm_widget tas5751m_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DAC", "HIFI Playback", SND_SOC_NOPM, 0, 0),
};

static const struct snd_soc_codec_driver soc_codec_dev_tas5751m = {
	.probe = tas5751m_probe,
	.remove = tas5751m_remove,
#ifdef CONFIG_PM
	.suspend = tas5751m_suspend,
	.resume = tas5751m_resume,
#endif
	.set_bias_level = tas5751m_set_bias_level,
	.component_driver = {
		.controls = tas5751m_snd_controls,
		.num_controls = ARRAY_SIZE(tas5751m_snd_controls),
		.dapm_widgets = tas5751m_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(tas5751m_dapm_widgets),
	}
};

/*
 *static const struct regmap_config tas5751m_regmap = {
 *	.reg_bits = 8,
 *	.val_bits = 8,
 *
 *	.max_register = tas5751m_REGISTER_COUNT,
 *	.reg_defaults = tas5751m_reg_defaults,
 *	.num_reg_defaults =
 *	sizeof(tas5751m_reg_defaults)/sizeof(tas5751m_reg_defaults[0]),
 *	.cache_type = REGCACHE_RBTREE,
 *};
 */

static int tas5751m_parse_dts(struct tas5751m_priv *tas5751m,
				struct device_node *np)
{
	int reset_pin = -1;
	const char *chnl_type;

	reset_pin = of_get_named_gpio(np, "reset_pin", 0);
	if (reset_pin < 0) {
		pr_err("%s fail to get reset pin from dts!\n", __func__);
	} else {
		pr_debug("%s pdata->reset_pin = %d!\n", __func__, reset_pin);
	}
	tas5751m->pdata->reset_pin = reset_pin;

	of_property_read_string(np, "channel_type", &chnl_type);
	if(chnl_type) {
		strcpy(tas5751m->channel_type, chnl_type);
	}

	return 0;
}

static int tas5751m_i2c_probe(struct i2c_client *i2c,
				const struct i2c_device_id *id)
{
	struct tas5751m_priv *tas5751m;
	struct tas57xx_platform_data *pdata;
	int ret;
	const char *codec_name = NULL;

	tas5751m = devm_kzalloc(&i2c->dev,
		sizeof(struct tas5751m_priv), GFP_KERNEL);
	if (!tas5751m)
		return -ENOMEM;


	pr_info("%s %d i2c:%p i2c->addr=0x%x\n", __func__, __LINE__, i2c, i2c->addr);
	/*
	 * tas5751m->regmap = devm_regmap_init_i2c(i2c, &tas5751m_regmap);
	 * if (IS_ERR(tas5751m->regmap)) {
	 *		ret = PTR_ERR(tas5751m->regmap);
	 *		dev_err(&i2c->dev,
	 *		"Failed to allocate register map: %d\n", ret);
	 *		return ret;
	 * }
	 */
	pdata = devm_kzalloc(&i2c->dev,
		sizeof(struct tas57xx_platform_data), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	tas5751m->pdata = pdata;

	tas5751m_parse_dts(tas5751m, i2c->dev.of_node);

	if (of_property_read_string(i2c->dev.of_node, "codec_name", &codec_name)) {
		pr_info("no codec name\n");
		ret = -1;
	}
	pr_info("codec_name=%s\n", codec_name);
	if (codec_name) {
		dev_set_name(&i2c->dev, "%s", codec_name);
	}
	tas5751m->i2c = i2c;
	i2c_set_clientdata(i2c, tas5751m);

	ret = snd_soc_register_codec(&i2c->dev,
		&soc_codec_dev_tas5751m, &tas5751m_dai, 1);
	if (ret != 0)
		dev_err(&i2c->dev, "Failed to register codec (%d)\n", ret);

	return ret;
}

static int tas5751m_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);

	return 0;
}

static const struct i2c_device_id tas5751m_i2c_id[] = {
	{ DEV_NAME, 0 },
	{}
};

static const struct of_device_id tas5751m_of_id[] = {
	{.compatible = "ti,tas5751m",},
	{ /* senitel */ }
};
MODULE_DEVICE_TABLE(of, tas5751m_of_id);

static struct i2c_driver tas5751m_i2c_driver = {
	.driver = {
		.name = DEV_NAME,
		.of_match_table = tas5751m_of_id,
		.owner = THIS_MODULE,
	},
	.probe = tas5751m_i2c_probe,
	.remove = tas5751m_i2c_remove,
	.id_table = tas5751m_i2c_id,
};

module_i2c_driver(tas5751m_i2c_driver);


MODULE_DESCRIPTION("ASoC Tas5751m driver");
MODULE_AUTHOR("AML MM team");
MODULE_LICENSE("GPL");

