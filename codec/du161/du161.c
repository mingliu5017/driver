/*
 * Driver for the DU161 CODECs
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
#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/gcd.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <linux/amlogic/aml_gpio_consumer.h>

#include "du161.h"

#define du161_RATES (SNDRV_PCM_RATE_8000 | \
               SNDRV_PCM_RATE_16000 | \
               SNDRV_PCM_RATE_22050 | \
               SNDRV_PCM_RATE_32000 | \
		       SNDRV_PCM_RATE_44100 | \
		       SNDRV_PCM_RATE_48000)

#define du161_FORMATS (SNDRV_PCM_FMTBIT_S16_LE)

/* codec private data */
struct du161_priv {
	struct regmap *regmap;
	struct snd_soc_codec *codec;
	int mute_pin;
};

static const struct reg_default du161_reg_defaults[] = {
	{ VERSION,                   0xEE82 },
	{ POWER_CONTROL,             0x0000 },
	{ PATH_MODE,                 0x0000 },
	{ SAMPLERATE,                0x0021 },
    { I2S_CTL,                   0x000A },
	{ PGA_CTL,                   0x0000 },
	{ PGA_LINEIN1_VOL,           0x3535 },
	{ PGA_LINEIN2_VOL,           0x2828 },
	{ PGA_MIC_VOL,               0x2A2A },
	{ ADC_DIGITAL_VOL,           0xFFFF },
	{ ADC_SR_ADJ,                0x0000 },
	{ ALC1,                      0x0302 },
	{ ALC2,                      0x0D72 },
	{ ALC_NOISE_GATE,            0x0000 },
    //{ DAC_CTL,                   0x0000 },
	{ DAC_DIGITAL_VOL,           0xFFFF },
	{ DAC_PGA_VOL,               0x2F2F },
	{ DAC_SR_ADJ,                0x0000 },
	{ EQ_CTL,                    0x0000 },
	{ EQ_PREGAIN,                0x0000 },
	{ EQ_F1_TYPE,                0x0000 },
	{ EQ_F1_F0,                  0x0000 },
	{ EQ_F1_Q,                   0x0000 },
	{ EQ_F1_GAIN,                0x0000 },
	{ EQ_F2_TYPE,                0x0000 },
	{ EQ_F2_F0,                  0x0000 },
	{ EQ_F2_Q,                   0x0000 },
	{ EQ_F2_GAIN,                0x0000 },
	{ EQ_F3_TYPE,                0x0000 },
	{ EQ_F3_F0,                  0x0000 },
	{ EQ_F3_Q,                   0x0000 },
	{ EQ_F3_GAIN,                0x0000 },
	{ EQ_F4_TYPE,                0x0000 },
	{ EQ_F4_F0,                  0x0000 },
	{ EQ_F4_Q,                   0x0000 },
	{ EQ_F4_GAIN,                0x0000 },
    { EQ_F5_TYPE,                0x0000 },
    { EQ_F5_F0,                  0x0000 },
    { EQ_F5_Q,                   0x0000 },
    { EQ_F5_GAIN,                0x0000 },
    { EQ_F6_TYPE,                0x0000 },
    { EQ_F6_F0,                  0x0000 },
    { EQ_F6_Q,                   0x0000 },
    { EQ_F6_GAIN,                0x0000 },
    { EQ_F7_TYPE,                0x0000 },
    { EQ_F7_F0,                  0x0000 },
    { EQ_F7_Q,                   0x0000 },
    { EQ_F7_GAIN,                0x0000 },
    { EQ_F8_TYPE,                0x0000 },
    { EQ_F8_F0,                  0x0000 },
    { EQ_F8_Q,                   0x0000 },
    { EQ_F8_GAIN,                0x0000 },
    { EQ_F9_TYPE,                0x0000 },
    { EQ_F9_F0,                  0x0000 },
    { EQ_F9_Q,                   0x0000 },
    { EQ_F9_GAIN,                0x0000 },
    { EQ_F10_TYPE,               0x0000 },
    { EQ_F10_F0,                 0x0000 },
    { EQ_F10_Q,                  0x0000 },
    { EQ_F10_GAIN,               0x0000 },
    { GPIO_CONFIG,               0xF000 },
    { GPIO_WRITE,                0xF000 },
    { GPIO_DATA,                 0x0000 },
    { ADC_LEVEL,                 0x0000 },
    { I2SIN_LEVEL,               0x0000 },
    { RESOURCE_USAGE,            0x0000 },
    { EFFECT_MODE,               0x0000 },
    { EFFECT_PREGAIN,            0x00FF },
    { EFFECT_BASS1,              0x00C8 },
    { EFFECT_BASS2,              0x0047 },
    { EFFECT_3D,                 0x0064 },
    { EFFECT_PITCH_SHIFTER,      0x0000 },
    { EFFECT_ECHO1,              0x0000 },
    { EFFECT_ECHO2,              0xA590 },
    { EFFECT_REVERB1,            0x00C8 },
    { EFFECT_REVERB2,            0x3232 },
    { EFFECT_REVERB3,            0x643C },
    { EFFECT_HOWLING_CONTROL,    0x0008 },
    { EFFECT_VOICE_CHANGER,      0x6482 },
    { EFFECT_NOISE_GATE1,        0x565A },
    { EFFECT_NOISE_GATE2,        0x0005 },
    { EFFECT_NOISE_GATE3,        0x0064 },
    { EFFECT_NOISE_GATE4,        0x01F4 },
    { EFFECT_MIX_VOL,            0xEFEF },
    { EFFECT_MIX_CTRL,           0x0000 },
    { EFFECT_DRC,                0x04B2 },
    { EFFECT_DRC_BAND1_P1,       0x6400 },
    { EFFECT_DRC_BAND1_P2,       0x8C01 },
    { EFFECT_DRC_BAND2_P1,       0x6400 },
    { EFFECT_DRC_BAND2_P2,       0x8C01 },
    { EFFECT_DRC_FULLBAND_P1,    0x6400 },
    { EFFECT_DRC_FULLBAND_P2,    0x8C01 },
    { EFFECT_DRC_PREGAIN1,       0x0000 },
    { EFFECT_DRC_PREGAIN2,       0x0000 },
    { EFFECT_DRC_MODE,           0x0000 },
    { EFFECT_DRC_FILTER_Q1,      0x02D4 },
    { EFFECT_DRC_FILTER_Q2,      0x02D4 },
    { EFFECT_AUTO_TUNE,          0x0040 },
    { EFFECT_NOISE_SUPPRESSOR1,  0x035A },
    { EFFECT_NOISE_SUPPRESSOR2,  0x0005 },
    { EFFECT_NOISE_SUPPRESSOR3,  0x01F4 }
};

static const struct reg_default du161_reg_config[] = {
	{ VERSION,                   0xEE82 },
	{ POWER_CONTROL,             0x0000 },
	{ PATH_MODE,                 0x0008 },
	{ SAMPLERATE,                0x00C0 },
    { I2S_CTL,                   0x0002 },
	{ PGA_CTL,                   0x0002 },
	{ PGA_LINEIN1_VOL,           0x3636 },
	{ PGA_LINEIN2_VOL,           0x2828 },
	{ PGA_MIC_VOL,               0x2A2A },
	{ ADC_DIGITAL_VOL,           0x0000 },
	{ ADC_SR_ADJ,                0x0000 },
	{ ALC1,                      0x0302 },
	{ ALC2,                      0x0D72 },
	{ ALC_NOISE_GATE,            0x0000 },
	{ DAC_DIGITAL_VOL,           0xFAFA },
	{ DAC_PGA_VOL,               0x2F2F },
	{ DAC_SR_ADJ,                0x0000 },
	{ EQ_CTL,                    0xC7CE },
	{ EQ_PREGAIN,                0xF800 },
	{ EQ_F1_TYPE,                0x0000 },
	{ EQ_F1_F0,                  0x076C },
	{ EQ_F1_Q,                   0x0C00 },
	{ EQ_F1_GAIN,                0xF601 },
	{ EQ_F2_TYPE,                0x0000 },
	{ EQ_F2_F0,                  0x00C8 },
	{ EQ_F2_Q,                   0x04CD },
	{ EQ_F2_GAIN,                0xFE01 },
	{ EQ_F3_TYPE,                0x0000 },
	{ EQ_F3_F0,                  0x0FA0 },
	{ EQ_F3_Q,                   0x0C00 },
	{ EQ_F3_GAIN,                0xFE01 },
	{ EQ_F4_TYPE,                0x0000 },
	{ EQ_F4_F0,                  0x00DC },
	{ EQ_F4_Q,                   0x0600 },
	{ EQ_F4_GAIN,                0xFD01 },
    { EQ_F5_TYPE,                0x0004 },
    { EQ_F5_F0,                  0x0037 },
    { EQ_F5_Q,                   0x02CD },
    { EQ_F5_GAIN,                0x0000 },
    { EQ_F6_TYPE,                0x0004 },
    { EQ_F6_F0,                  0x001E },
    { EQ_F6_Q,                   0x02D4 },
    { EQ_F6_GAIN,                0x0000 },
    { EQ_F7_TYPE,                0x0000 },
    { EQ_F7_F0,                  0x0320 },
    { EQ_F7_Q,                   0x0A00 },
    { EQ_F7_GAIN,                0x0200 },
    { EQ_F8_TYPE,                0x0000 },
    { EQ_F8_F0,                  0x004B },
    { EQ_F8_Q,                   0x0333 },
    { EQ_F8_GAIN,                0x0800 },
    { EQ_F9_TYPE,                0x0000 },
    { EQ_F9_F0,                  0x4650 },
    { EQ_F9_Q,                   0x0600 },
    { EQ_F9_GAIN,                0x0400 },
    { EQ_F10_TYPE,               0x0002 },
    { EQ_F10_F0,                 0x09C4 },
    { EQ_F10_Q,                  0x02D4 },
    { EQ_F10_GAIN,               0x0580 },
    //{ GPIO_CONFIG,               0x2005 },
    //{ GPIO_WRITE,                0x2001 },
    //{ GPIO_DATA,                 0x03BF },
    //{ ADC_LEVEL,                 0x0000 },
    //{ I2SIN_LEVEL,               0x0000 },
    //{ RESOURCE_USAGE,            0x0000 },
    { EFFECT_MODE,               0x0000 },
    { EFFECT_PREGAIN,            0x00FF },
    { EFFECT_BASS1,              0x00C8 },
    { EFFECT_BASS2,              0x0047 },
    { EFFECT_3D,                 0x0064 },
    { EFFECT_PITCH_SHIFTER,      0x0000 },
    { EFFECT_ECHO1,              0x0000 },
    { EFFECT_ECHO2,              0xA590 },
    { EFFECT_REVERB1,            0x00C8 },
    { EFFECT_REVERB2,            0x3232 },
    { EFFECT_REVERB3,            0x6432 },
    { EFFECT_HOWLING_CONTROL,    0x0008 },
    { EFFECT_VOICE_CHANGER,      0x6482 },
    { EFFECT_NOISE_GATE1,        0x565A },
    { EFFECT_NOISE_GATE2,        0x0005 },
    { EFFECT_NOISE_GATE3,        0x0064 },
    { EFFECT_NOISE_GATE4,        0x01F4 },
    { EFFECT_MIX_VOL,            0xEFEF },
    { EFFECT_MIX_CTRL,           0x0000 },
    { EFFECT_DRC,                0x0323 },
    { EFFECT_DRC_BAND1_P1,       0x0810 },
    { EFFECT_DRC_BAND1_P2,       0xB90A },
    { EFFECT_DRC_BAND2_P1,       0x080C },
    { EFFECT_DRC_BAND2_P2,       0xBE0A },
    { EFFECT_DRC_FULLBAND_P1,    0x0A1F },
    { EFFECT_DRC_FULLBAND_P2,    0xC80A },
    { EFFECT_DRC_PREGAIN1,       0x08C0 },
    { EFFECT_DRC_PREGAIN2,       0x0900 },
    { EFFECT_DRC_MODE,           0x0005 },
    { EFFECT_DRC_FILTER_Q1,      0x02D4 },
    { EFFECT_DRC_FILTER_Q2,      0x02D4 },
    { EFFECT_AUTO_TUNE,          0x0040 },
    { EFFECT_NOISE_SUPPRESSOR1,  0x035A },
    { EFFECT_NOISE_SUPPRESSOR2,  0x0005 },
    { EFFECT_NOISE_SUPPRESSOR3,  0x01F4 }
};

static const DECLARE_TLV_DB_SCALE(mvol_tlv, -72000, 375, 0);

static const struct snd_kcontrol_new du161_controls[] = {
	/* Speaker Output Volume */
	SOC_DOUBLE_TLV("Digital Playback Volume", DAC_DIGITAL_VOL,
		DU161_L_VOL_SFT, DU161_R_VOL_SFT, 0xFA, 0, mvol_tlv),
    /* output mute */
    SOC_DOUBLE("Playback Mute Switch", DAC_CTL,
			DU161_L_MUTE_SHIFT, DU161_R_MUTE_SHIFT, 1, 1),
};

static const struct snd_soc_dapm_widget du161_dapm_widgets[] = {
    SND_SOC_DAPM_DAC("DAC", "HIFI Playback", SND_SOC_NOPM, 0, 0),
};

static int du161_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params,
			     struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	unsigned int val=0;
	int ret;

	switch (params_rate(params)) {
	case 48000:
		val = DU161_SR_48;
		break;
	case 44100:
		val = DU161_SR_441;
		break;
	case 32000:
		val = DU161_SR_32;
		break;
	case 24000:
		val = DU161_SR_24;
		break;
	case 22050:
		val = DU161_SR_2205;
		break;
	case 16000:
		val = DU161_SR_16;
		break;
	case 12000:
		val = DU161_SR_12;
		break;
	case 11025:
		val = DU161_SR_11025;
		break;
	case 8000:
		val = DU161_SR_8;

	default:
		dev_err(codec->dev, "Invalid DAI sample rate!\n");
		return -EINVAL;
	}

	ret = snd_soc_update_bits(codec, SAMPLERATE, DU161_SR_MASK, val);
	if (ret < 0)
		return ret;

	return 0;
}

static int du161_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;
	unsigned int val = 0;
	int ret;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		val |= DU161_MODE_MASTER;
		break;
	default:
		dev_err(codec->dev, "Invalid DAI format\n");
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		val |= DU161_FORMAT_DAC_DIF_I2S;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		val |= DU161_FORMAT_DAC_DIF_RJ;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		val |= DU161_FORMAT_DAC_DIF_LJ;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		val |= DU161_MODE1_DAC_DIF_DSPA;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		val |= DU161_MODE1_DAC_DIF_DSPB;
		break;
	default:
		dev_err(codec->dev, "Invalid DAI format\n");
		return -EINVAL;
	}

	ret = snd_soc_update_bits(codec, I2S_CTL, DU161_I2SCTL_MS_MASK|DU161_I2SCTL_FORMAT_MASK, val);
	if (ret < 0)
		return ret;

	return 0;
}

static int du161_digital_mute(struct snd_soc_dai *dai, int mute, int stream)
{
	struct snd_soc_codec *codec = dai->codec;
	struct du161_priv *du161 = snd_soc_codec_get_drvdata(codec);
	int ret = 0;

	if (stream != SNDRV_PCM_STREAM_PLAYBACK)
		return 0;

	printk("du161 mute state:%d\n", mute);
    /*DSP MUTE*/
	ret = snd_soc_update_bits(codec, DAC_CTL, 0x00C0, mute ? 0x00C0 : 0);
	if (ret < 0)
		return ret;

	/*amplifier MUTE*/
	ret = gpio_direction_output(du161->mute_pin, mute ? GPIOF_OUT_INIT_LOW : GPIOF_OUT_INIT_HIGH);
	if (ret < 0)
		return ret;

	return 0;
}

static const struct snd_soc_dai_ops du161_dai_ops = {
	.hw_params = du161_hw_params,
	.set_fmt = du161_set_fmt,
    .mute_stream = du161_digital_mute,
};

static struct snd_soc_dai_driver du161_dai = {
	.name = "du161-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = du161_RATES,
		.formats = du161_FORMATS,
	},
	.ops = &du161_dai_ops,
};

static int du161_codec_probe(struct snd_soc_codec *codec)
{
	int loop = 0;

    /*du161 mute*/
    snd_soc_write(codec, DAC_CTL, 0x00C1);

    /*du161 init register*/
	for (loop = PATH_MODE; loop < ARRAY_SIZE(du161_reg_config); loop++) {
        //printk("du161 writ reg%d = 0x%x\n", du161_reg_config[loop].reg, du161_reg_config[loop].def);
		snd_soc_write(codec, du161_reg_config[loop].reg, du161_reg_config[loop].def);
	}

    printk("du161 init over!!!\n");
	return 0;
}

static int du161_codec_remove(struct snd_soc_codec *codec)
{
    /*芯片进入睡眠模式*/
    snd_soc_write(codec, POWER_CONTROL, 0x0002);
	return 0;
}

static int du161_set_bias_level(struct snd_soc_codec *codec,
				  enum snd_soc_bias_level level)
{
	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
		break;

	case SND_SOC_BIAS_STANDBY:
		break;

	case SND_SOC_BIAS_OFF:
		break;
	}
 
    codec->component.dapm.bias_level = level;

	return 0;
}

static struct snd_soc_codec_driver du161_codec_driver = {
	.probe = du161_codec_probe,
	.remove = du161_codec_remove,
    .set_bias_level = du161_set_bias_level,

	.component_driver = {
		.controls		    = du161_controls,
		.num_controls		= ARRAY_SIZE(du161_controls),
		.dapm_widgets		= du161_dapm_widgets,
		.num_dapm_widgets	= ARRAY_SIZE(du161_dapm_widgets),
	},
};

static bool du161_readable(struct device *dev, unsigned int reg)
{
	return true;
}

static bool du161_volatile(struct device *dev, unsigned int reg)
{
    return true;
}

const struct regmap_config du161_regmap = {
	.reg_bits = 8,
	.val_bits = 16,
    .use_single_rw = true,

	.readable_reg = du161_readable,
	.volatile_reg = du161_volatile,

	.max_register = DU161_MAX_REGISTER,
	.reg_defaults = du161_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(du161_reg_defaults),
	.cache_type = REGCACHE_RBTREE,
};

EXPORT_SYMBOL_GPL(du161_regmap);

static void du161_parse_dt(struct du161_priv *du161, struct device_node *np)
{
	int mute_pin = -1;

	mute_pin = of_get_named_gpio(np, "mute-gpio", 0);
	if (mute_pin < 0) {
		printk("fail to get mute pin from dts!\n");
		return;
	} 

	du161->mute_pin = mute_pin;

	return;
}

int du161_probe(struct device *dev, struct regmap *regmap)
{
	struct du161_priv *du161;
	int ret;
    unsigned int du161_version;

	du161 = devm_kzalloc(dev, sizeof(struct du161_priv), GFP_KERNEL);
	if (!du161)
		return -ENOMEM;

	dev_set_drvdata(dev, du161);
	du161->regmap = regmap;
	du161_parse_dt(du161, dev->of_node);

	if (gpio_is_valid(du161->mute_pin)) {
		ret = devm_gpio_request_one(dev, du161->mute_pin,
				GPIOF_OUT_INIT_LOW, "du161 mute");
		if (ret != 0){
			printk("DU161 devm_gpio_request_one faild,ret = %d!\n", ret);
			return ret;
	    }else{
	        printk("DU161 mute gpio %d request sucess!\n",du161->mute_pin);
	    }
	}

	/* 读取IC版本 */
	ret = regmap_read(regmap, VERSION, &du161_version);
	if (ret != 0) {
		dev_err(dev, "Failed to read version: %d\n", ret);
		return ret;
	}
    printk("DU161 i2c probe version:0x%x\n",du161_version);

	ret = snd_soc_register_codec(dev, &du161_codec_driver, &du161_dai, 1);
	if (ret != 0) {
		dev_err(dev, "Failed to register CODEC: %d\n", ret);
		return ret;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(du161_probe);

void du161_remove(struct device *dev)
{
	snd_soc_unregister_codec(dev);
}
EXPORT_SYMBOL_GPL(du161_remove);

MODULE_DESCRIPTION("ASoC DU161 codec driver for ALSA");
MODULE_AUTHOR("Liu Ming <ming.liu@3nod.com>");
MODULE_LICENSE("GPL v2");
