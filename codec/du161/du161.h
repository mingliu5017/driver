/*
 * Driver for the DU161 CODECs
 *
 * Author:	liuming <ming.liu@3nod.com>
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

#ifndef _SND_SOC_DU161
#define _SND_SOC_DU161

#include <linux/pm.h>
#include <linux/regmap.h>

#define VERSION            0x0
#define POWER_CONTROL      1
#define PATH_MODE          2
#define SAMPLERATE         3
#define I2S_CTL            4
#define PGA_CTL            5
#define PGA_LINEIN1_VOL    6
#define PGA_LINEIN2_VOL    7
#define PGA_MIC_VOL        8
#define ADC_DIGITAL_VOL    9
#define ADC_SR_ADJ         10
#define ALC1               11
#define ALC2               12
#define ALC_NOISE_GATE     13
#define DAC_CTL            14
#define DAC_DIGITAL_VOL    15
#define DAC_PGA_VOL        16
#define DAC_SR_ADJ         17
#define EQ_CTL             18
#define EQ_PREGAIN         19
#define EQ_F1_TYPE         20
#define EQ_F1_F0           21
#define EQ_F1_Q            22
#define EQ_F1_GAIN         23
#define EQ_F2_TYPE         24
#define EQ_F2_F0           25
#define EQ_F2_Q            26
#define EQ_F2_GAIN         27
#define EQ_F3_TYPE         28
#define EQ_F3_F0           29
#define EQ_F3_Q            30
#define EQ_F3_GAIN         31
#define EQ_F4_TYPE         32
#define EQ_F4_F0           33
#define EQ_F4_Q            34
#define EQ_F4_GAIN         35
#define EQ_F5_TYPE         36
#define EQ_F5_F0           37
#define EQ_F5_Q            38
#define EQ_F5_GAIN         39
#define EQ_F6_TYPE         40
#define EQ_F6_F0           41
#define EQ_F6_Q            42
#define EQ_F6_GAIN         43
#define EQ_F7_TYPE         44
#define EQ_F7_F0           45
#define EQ_F7_Q            46
#define EQ_F7_GAIN         47
#define EQ_F8_TYPE         48
#define EQ_F8_F0           49
#define EQ_F8_Q            50
#define EQ_F8_GAIN         51
#define EQ_F9_TYPE         52
#define EQ_F9_F0           53
#define EQ_F9_Q            54
#define EQ_F9_GAIN         55
#define EQ_F10_TYPE        56
#define EQ_F10_F0          57
#define EQ_F10_Q           58
#define EQ_F10_GAIN        59
//RESERVED                    60
#define GPIO_CONFIG        61
#define GPIO_WRITE         62
#define GPIO_DATA          63
//RESERVED                    64
#define ADC_LEVEL          65
#define I2SIN_LEVEL        66
//RESERVED                    67
#define RESOURCE_USAGE     68
#define EFFECT_MODE        69
//RESERVED                    70
#define EFFECT_PREGAIN     71
#define EFFECT_BASS1       72
#define EFFECT_BASS2       73
#define EFFECT_3D          74
#define EFFECT_PITCH_SHIFTER 75
#define EFFECT_ECHO1       76
#define EFFECT_ECHO2       77
#define EFFECT_REVERB1     78
#define EFFECT_REVERB2     79
#define EFFECT_REVERB3     80
#define EFFECT_HOWLING_CONTROL 81
#define EFFECT_VOICE_CHANGER   82
#define EFFECT_NOISE_GATE1     83
#define EFFECT_NOISE_GATE2     84
#define EFFECT_NOISE_GATE3     85
#define EFFECT_NOISE_GATE4     86
#define EFFECT_MIX_VOL         87
#define EFFECT_MIX_CTRL        88
#define EFFECT_DRC             89
#define EFFECT_DRC_BAND1_P1    90
#define EFFECT_DRC_BAND1_P2    91
#define EFFECT_DRC_BAND2_P1    92
#define EFFECT_DRC_BAND2_P2    93
#define EFFECT_DRC_FULLBAND_P1 94
#define EFFECT_DRC_FULLBAND_P2 95
#define EFFECT_DRC_PREGAIN1    96
#define EFFECT_DRC_PREGAIN2    97
#define EFFECT_DRC_MODE        98
#define EFFECT_DRC_FILTER_Q1   99
#define EFFECT_DRC_FILTER_Q2   100
#define EFFECT_AUTO_TUNE       101
#define EFFECT_NOISE_SUPPRESSOR1 102
#define EFFECT_NOISE_SUPPRESSOR2 103
#define EFFECT_NOISE_SUPPRESSOR3 104

#define DU161_MAX_REGISTER   104

#define DU161_L_VOL_SFT			8
#define DU161_R_VOL_SFT		    0

#define DU161_L_MUTE_SHIFT		7
#define DU161_R_MUTE_SHIFT		6

#define DU161_I2SCTL_MS_MASK	    0x0008
#define DU161_I2SCTL_FORMAT_MASK	0x0007
#define DU161_FORMAT_DAC_DIF_RJ		0x00
#define DU161_FORMAT_DAC_DIF_LJ		0x01
#define DU161_FORMAT_DAC_DIF_I2S	0x02
#define DU161_MODE1_DAC_DIF_DSPB	0x03
#define DU161_MODE1_DAC_DIF_DSPA	0x04

#define DU161_MODE_MASTER	(1 << 3)

#define DU161_SR_48		0x00
#define DU161_SR_441	0x01
#define DU161_SR_32		0x02
#define DU161_SR_24		0x03
#define DU161_SR_2205	0x04
#define DU161_SR_16		0x05
#define DU161_SR_12		0x06
#define DU161_SR_11025	0x07
#define DU161_SR_8 		0x08
#define DU161_SR_MASK	0x0F

extern const struct regmap_config du161_regmap;

int du161_probe(struct device *dev, struct regmap *regmap);
void du161_remove(struct device *dev);

#endif
