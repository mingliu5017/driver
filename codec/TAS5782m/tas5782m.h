/*
 * sound/soc/codecs/amlogic/tas5782m.c
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
#ifndef _TAS5782M_H
#define _TAS5782M_H

#include <linux/regmap.h>


#define CFG_META_SWITCH (255)
#define CFG_META_DELAY  (254)
#define CFG_META_BURST  (253)

#define WORK_MODE_I2S 0
#define WORK_MODE_TDM 1

#define WORK_MODE WORK_MODE_I2S

static const struct reg_default tas5782m_reg_defaults[] = {
	//program memory
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x02, 0x11 },
	{ CFG_META_BURST, 2 },
	{ 0x01, 0x11 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x03, 0x11 },
	{ CFG_META_BURST, 2 },
	{ 0x2a, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x25, 0x18 },
	{ CFG_META_BURST, 2 },
	{ 0x0d, 0x10 },
	{ CFG_META_BURST, 2 },
	{ 0x02, 0x00 },

	//Sample rate update
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x02, 0x80 },

	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x00 },

	// speed 03-48k 04-96k
	//dynamically reading speed
	{ CFG_META_BURST, 2 },
	{ 0x22, 0x03 },

	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x02, 0x00 },

	//write coefficients of various components
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x1c, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 5 },
	{ 0x28, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 5 },
	{ 0x34, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 5 },
	{ 0x40, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1f },
	{ CFG_META_BURST, 5 },
	{ 0x24, 0x00 },
	{ 0x20, 0xc4 },
	{ 0x9c, 0x00 },
	{ CFG_META_BURST, 5 },
	{ 0x30, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x11 },
	{ CFG_META_BURST, 21 },
	{ 0x58, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x12 },
	{ CFG_META_BURST, 21 },
	{ 0x44, 0xff },
	{ 0x84, 0x59 },
	{ 0x16, 0xff },
	{ 0x84, 0x59 },
	{ 0x16, 0xff },
	{ 0x84, 0x59 },
	{ 0x16, 0x70 },
	{ 0x46, 0x2b },
	{ 0x3b, 0x9d },
	{ 0x85, 0x0d },
	{ 0xe2, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x13 },
	{ CFG_META_BURST, 21 },
	{ 0x30, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x13 },
	{ CFG_META_BURST, 21 },
	{ 0x58, 0x4f },
	{ 0x9d, 0xf9 },
	{ 0x35, 0xb0 },
	{ 0x62, 0x06 },
	{ 0xcb, 0x4f },
	{ 0x9d, 0xf9 },
	{ 0x35, 0x49 },
	{ 0xe6, 0x9d },
	{ 0x16, 0xd5 },
	{ 0x55, 0x55 },
	{ 0x56, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x14 },
	{ CFG_META_BURST, 21 },
	{ 0x08, 0x7c },
	{ 0xb6, 0xa4 },
	{ 0xbc, 0x83 },
	{ 0x49, 0x5b },
	{ 0x44, 0x7c },
	{ 0xb6, 0xa4 },
	{ 0xbc, 0x7c },
	{ 0xb1, 0x2c },
	{ 0x1e, 0x86 },
	{ 0x87, 0xc5 },
	{ 0x49, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x14 },
	{ CFG_META_BURST, 21 },
	{ 0x1c, 0x05 },
	{ 0xb7, 0x5c },
	{ 0x1f, 0x05 },
	{ 0xb7, 0x5c },
	{ 0x1f, 0x05 },
	{ 0xb7, 0x5c },
	{ 0x1f, 0x49 },
	{ 0xe6, 0x9d },
	{ 0x16, 0xd5 },
	{ 0x55, 0x55 },
	{ 0x56, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x14 },
	{ CFG_META_BURST, 21 },
	{ 0x58, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x14 },
	{ CFG_META_BURST, 21 },
	{ 0x6c, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x15 },
	{ CFG_META_BURST, 21 },
	{ 0x08, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x15 },
	{ CFG_META_BURST, 21 },
	{ 0x1c, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x15 },
	{ CFG_META_BURST, 21 },
	{ 0x30, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x15 },
	{ CFG_META_BURST, 21 },
	{ 0x44, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x15 },
	{ CFG_META_BURST, 21 },
	{ 0x58, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x15 },
	{ CFG_META_BURST, 21 },
	{ 0x6c, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x16 },
	{ CFG_META_BURST, 21 },
	{ 0x08, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x16 },
	{ CFG_META_BURST, 21 },
	{ 0x1c, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x16 },
	{ CFG_META_BURST, 21 },
	{ 0x30, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x16 },
	{ CFG_META_BURST, 21 },
	{ 0x44, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x16 },
	{ CFG_META_BURST, 21 },
	{ 0x58, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x16 },
	{ CFG_META_BURST, 21 },
	{ 0x6c, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x17 },
	{ CFG_META_BURST, 21 },
	{ 0x08, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x17 },
	{ CFG_META_BURST, 21 },
	{ 0x1c, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x17 },
	{ CFG_META_BURST, 21 },
	{ 0x30, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x17 },
	{ CFG_META_BURST, 21 },
	{ 0x44, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x17 },
	{ CFG_META_BURST, 21 },
	{ 0x58, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x17 },
	{ CFG_META_BURST, 21 },
	{ 0x6c, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x18 },
	{ CFG_META_BURST, 21 },
	{ 0x08, 0x08 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x18 },
	{ CFG_META_BURST, 21 },
	{ 0x1c, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x18 },
	{ CFG_META_BURST, 21 },
	{ 0x30, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x18 },
	{ CFG_META_BURST, 21 },
	{ 0x44, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x18 },
	{ CFG_META_BURST, 21 },
	{ 0x58, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x18 },
	{ CFG_META_BURST, 21 },
	{ 0x6c, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x19 },
	{ CFG_META_BURST, 21 },
	{ 0x08, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x19 },
	{ CFG_META_BURST, 21 },
	{ 0x1c, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x19 },
	{ CFG_META_BURST, 21 },
	{ 0x30, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x19 },
	{ CFG_META_BURST, 21 },
	{ 0x44, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x19 },
	{ CFG_META_BURST, 21 },
	{ 0x58, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x19 },
	{ CFG_META_BURST, 21 },
	{ 0x6c, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1a },
	{ CFG_META_BURST, 21 },
	{ 0x08, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1a },
	{ CFG_META_BURST, 21 },
	{ 0x1c, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1a },
	{ CFG_META_BURST, 21 },
	{ 0x30, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1a },
	{ CFG_META_BURST, 21 },
	{ 0x44, 0x08 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1a },
	{ CFG_META_BURST, 5 },
	{ 0x58, 0x00 },
	{ 0xe2, 0xc4 },
	{ 0x6b, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1a },
	{ CFG_META_BURST, 21 },
	{ 0x6c, 0x00 },
	{ 0x06, 0xd3 },
	{ 0x72, 0x00 },
	{ 0x02, 0xbb },
	{ 0x06, 0x00 },
	{ 0x03, 0x69 },
	{ 0xc5, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1b },
	{ CFG_META_BURST, 21 },
	{ 0x08, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0xf9 },
	{ 0xda, 0xbc },
	{ 0x21, 0xfc },
	{ 0x58, 0x8b },
	{ 0x89, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1b },
	{ CFG_META_BURST, 41 },
	{ 0x1c, 0x00 },
	{ 0x06, 0xd3 },
	{ 0x72, 0x00 },
	{ 0x02, 0xbb },
	{ 0x06, 0x00 },
	{ 0x03, 0x69 },
	{ 0xc5, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0xf9 },
	{ 0xda, 0xbc },
	{ 0x21, 0xfc },
	{ 0x58, 0x8b },
	{ 0x89, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1b },
	{ CFG_META_BURST, 41 },
	{ 0x44, 0x00 },
	{ 0x06, 0xd3 },
	{ 0x72, 0x00 },
	{ 0x02, 0xbb },
	{ 0x06, 0x00 },
	{ 0x03, 0x69 },
	{ 0xc5, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0xf9 },
	{ 0xda, 0xbc },
	{ 0x21, 0xfc },
	{ 0x58, 0x8b },
	{ 0x89, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1c },
	{ CFG_META_BURST, 21 },
	{ 0x6c, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 21 },
	{ 0x08, 0x7f },
	{ 0xff, 0xff },
	{ 0xff, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x1c, 0x00 },
	{ 0x06, 0xd3 },
	{ 0x72, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x20, 0x1c },
	{ 0x1b, 0xf0 },
	{ 0x41, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x24, 0x04 },
	{ 0x0c, 0x37 },
	{ 0x14, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x2c, 0x00 },
	{ 0x80, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x34, 0x40 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x38, 0x40 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x40, 0x00 },
	{ 0x80, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x44, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x48, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x58, 0x00 },
	{ 0x00, 0x00 },
	{ 0x01, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x5c, 0x00 },
	{ 0x80, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x60, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x64, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x68, 0x00 },
	{ 0x80, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x74, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x78, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1d },
	{ CFG_META_BURST, 5 },
	{ 0x7c, 0x00 },
	{ 0x80, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x08, 0x00 },
	{ 0x80, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x0c, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x10, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x14, 0x00 },
	{ 0x80, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x18, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x1c, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x20, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x24, 0x00 },
	{ 0x80, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x28, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x2c, 0x00 },
	{ 0x80, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x30, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x34, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x38, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x3c, 0x00 },
	{ 0x80, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x40, 0x00 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },

	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x44, 0x00 },
	{ 0x00, 0x00 },
	{ 0x1b, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x48, 0x00 },
	{ 0x00, 0x00 },
	{ 0x1b, 0x00 },

	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x50, 0x40 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x54, 0x04 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1e },
	{ CFG_META_BURST, 5 },
	{ 0x58, 0x04 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1f },
	{ CFG_META_BURST, 5 },
	{ 0x14, 0x00 },
	{ 0xce, 0xc0 },
	{ 0x8a, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1f },
	{ CFG_META_BURST, 5 },
	{ 0x18, 0x0a },
	{ 0x0a, 0xae },
	{ 0xd2, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1f },
	{ CFG_META_BURST, 5 },
	{ 0x1c, 0x00 },
	{ 0x03, 0x69 },
	{ 0xd0, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1f },
	{ CFG_META_BURST, 5 },
	{ 0x20, 0x40 },
	{ 0x00, 0x00 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1f },
	{ CFG_META_BURST, 5 },
	{ 0x28, 0x75 },
	{ 0xf5, 0x51 },
	{ 0x2e, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x1f },
	{ CFG_META_BURST, 5 },
	{ 0x2c, 0x00 },
	{ 0x00, 0x57 },
	{ 0x62, 0x00 },

	//swap command
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x8c },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x23 },
	{ CFG_META_BURST, 5 },
	{ 0x14, 0x00 },
	{ 0x00, 0x00 },
	{ 0x01, 0x00 },

	//register tuning
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x07, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x08, 0x20 },
	{ CFG_META_BURST, 2 },
	{ 0x55, 0x07 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x3d, 0x30 },
	{ CFG_META_BURST, 2 },
	{ 0x3e, 0x30 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x01 },
	{ CFG_META_BURST, 2 },
	{ 0x02, 0x00 },

	//Unmute the device
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x03, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x2a, 0x11 },

	//start set volume
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x3d, 0x4f },

	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x7f, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x00, 0x00 },
	{ CFG_META_BURST, 2 },
	{ 0x3e, 0x4f },
	//end set volume
};

#endif
