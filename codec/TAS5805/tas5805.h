#ifndef _TAS5805_H_
#define _TAS5805_H_

#define DDX_CLOCK_CTL                               0x00
#define DDX_DEVICE_ID                               0x01
#define DDX_ERROR_STATUS                            0x02
#define DDX_SYS_CTL_1                               0x03
#define DDX_SERIAL_DATA_INTERFACE                   0x04
#define DDX_SYS_CTL_2                               0x05
#define DDX_SOFT_MUTE                               0x06
#define DDX_MASTER_VOLUME                           0x07
#define DDX_CHANNEL1_VOL                            0x08
#define DDX_CHANNEL2_VOL                            0x09
#define DDX_CHANNEL3_VOL                            0x0A
#define DDX_VOLUME_CONFIG                           0x0E

#define DDX_MODULATION_LIMIT                        0x10
#define DDX_IC_DELAY_CHANNEL_1                      0x11
#define DDX_IC_DELAY_CHANNEL_2                      0x12
#define DDX_IC_DELAY_CHANNEL_3                      0x13
#define DDX_IC_DELAY_CHANNEL_4                      0x14
#define DDX_PWM_SHUTDOWN_GROUP                      0x19
#define DDX_START_STOP_PERIOD                       0x1A
#define DDX_OSC_TRIM                                0x1B
#define DDX_BKND_ERR                                0x1C
#define DDX_NUM_BYTE_REG                            0x1D

static const u8 tas5805_low_volume_eq[][2] = {
	{ 0x00, 0x00 },
	{ 0x7f, 0xaa }, //book aa
	{ 0x00, 0x24 }, //page 24
	{ 0x40, 0x08 },
	{ 0x41, 0x0f },
	{ 0x42, 0xe4 },
	{ 0x43, 0x05 },
	{ 0x44, 0xf0 },
	{ 0x45, 0x0c },
	{ 0x46, 0xa4 },
	{ 0x47, 0xd4 },
	{ 0x48, 0x07 },
	{ 0x49, 0xe3 },
	{ 0x4a, 0xa3 },
	{ 0x4b, 0x0a },
	{ 0x4c, 0x0f },
	{ 0x4d, 0xf3 },
	{ 0x4e, 0x5b },
	{ 0x4f, 0x2c },
	{ 0x50, 0xf8 },
	{ 0x51, 0x0c },
	{ 0x52, 0x78 },
	{ 0x53, 0xf1 }
};

static const u8 tas5805_mid_volume_eq[][2] = {
	{ 0x00, 0x00 },
	{ 0x7f, 0xaa }, //book aa
	{ 0x00, 0x24 }, //page 24
	{ 0x40, 0x08 },
	{ 0x41, 0x0b },
	{ 0x42, 0x56 },
	{ 0x43, 0xff },
	{ 0x44, 0xf0 },
	{ 0x45, 0x0c },
	{ 0x46, 0xa4 },
	{ 0x47, 0xd4 },
	{ 0x48, 0x07 },
	{ 0x49, 0xe8 },
	{ 0x4a, 0x30 },
	{ 0x4b, 0x10 },
	{ 0x4c, 0x0f },
	{ 0x4d, 0xf3 },
	{ 0x4e, 0x5b },
	{ 0x4f, 0x2c },
	{ 0x50, 0xf8 },
	{ 0x51, 0x0c },
	{ 0x52, 0x78 },
	{ 0x53, 0xf1 }
};

static const u8 tas5805_high_volume_eq[][2] = {
	{ 0x00, 0x00 },
	{ 0x7f, 0xaa }, //book aa
	{ 0x00, 0x24 }, //page 24
	{ 0x40, 0x08 },
	{ 0x41, 0x07 },
	{ 0x42, 0xb9 },
	{ 0x43, 0x95 },
	{ 0x44, 0xf0 },
	{ 0x45, 0x0c },
	{ 0x46, 0xa4 },
	{ 0x47, 0xd4 },
	{ 0x48, 0x07 },
	{ 0x49, 0xeb },
	{ 0x4a, 0xcd },
	{ 0x4b, 0x7a },
	{ 0x4c, 0x0f },
	{ 0x4d, 0xf3 },
	{ 0x4e, 0x5b },
	{ 0x4f, 0x2c },
	{ 0x50, 0xf8 },
	{ 0x51, 0x0c },
	{ 0x52, 0x78 },
	{ 0x53, 0xf1 }
};

static const u8 tas5805_regs[][2] = {
    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x03, 0x02 },
    { 0x01, 0x11 },
    { 0x00, 0x00 },
    { 0x00, 0x00 },
    { 0x00, 0x00 },
    { 0x00, 0x00 },
    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x03, 0x02 },
    { 0xfe, 0x05 },
    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x03, 0x00 },
    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x46, 0x11 },

    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x03, 0x02 },
    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x78, 0x80 },

    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x61, 0x0b },
    { 0x60, 0x01 },
    { 0x7d, 0x11 },
    { 0x7e, 0xff },
    { 0x00, 0x01 },
    { 0x51, 0x05 },


    { 0x00, 0x00 },
    { 0x02, 0x04 },
    { 0x53, 0x00 },
    { 0x54, 0x12 },
    { 0x00, 0x00 },
    { 0x00, 0x00 },
    { 0x00, 0x00 },
    { 0x00, 0x00 },

    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x66, 0x87 },
    { 0x7f, 0x8c },
    { 0x00, 0x29 },
    { 0x18, 0x00 },
    { 0x19, 0x40 },
    { 0x1a, 0x26 },
    { 0x1b, 0xe7 },
    { 0x1c, 0x00 },
    { 0x1d, 0x40 },
    { 0x1e, 0x26 },
    { 0x1f, 0xe7 },
    { 0x20, 0x00 },
    { 0x21, 0x40 },
    { 0x22, 0x26 },
    { 0x23, 0xe7 },
    { 0x24, 0x00 },
    { 0x25, 0x40 },
    { 0x26, 0x26 },
    { 0x27, 0xe7 },
    { 0x00, 0x2a },
    { 0x24, 0x00 },
    { 0x25, 0xff },
    { 0x26, 0x64 },
    { 0x27, 0xc1 },
    { 0x28, 0x00 },
    { 0x29, 0xff },
    { 0x2a, 0x64 },
    { 0x2b, 0xc1 },
    { 0x30, 0x01 },
    { 0x31, 0x53 },
    { 0x32, 0x8f },
    { 0x33, 0xcc },
    { 0x00, 0x2c },
    { 0x0c, 0x00 },
    { 0x0d, 0x00 },
    { 0x0e, 0x00 },
    { 0x0f, 0x00 },
    { 0x10, 0x00 },
    { 0x11, 0x00 },
    { 0x12, 0x00 },
    { 0x13, 0x00 },
    { 0x14, 0x00 },
    { 0x15, 0x80 },
    { 0x16, 0x00 },
    { 0x17, 0x00 },
    { 0x18, 0x00 },
    { 0x19, 0x00 },
    { 0x1a, 0x00 },
    { 0x1b, 0x00 },
    { 0x1c, 0x00 },
    { 0x1d, 0x80 },
    { 0x1e, 0x00 },
    { 0x1f, 0x00 },
    { 0x20, 0x00 },
    { 0x21, 0x00 },
    { 0x22, 0x00 },
    { 0x23, 0x00 },
    { 0x28, 0x00 },
    { 0x29, 0x80 },
    { 0x2a, 0x00 },
    { 0x2b, 0x00 },
    { 0x2c, 0x00 },
    { 0x2d, 0x00 },
    { 0x2e, 0x00 },
    { 0x2f, 0x00 },
    { 0x34, 0x00 },
    { 0x35, 0x80 },
    { 0x36, 0x00 },
    { 0x37, 0x00 },
    { 0x38, 0x00 },
    { 0x39, 0x00 },
    { 0x3a, 0x00 },
    { 0x3b, 0x00 },
    { 0x48, 0x00 },
    { 0x49, 0x80 },
    { 0x4a, 0x00 },
    { 0x4b, 0x00 },
    { 0x4c, 0x00 },
    { 0x4d, 0x00 },
    { 0x4e, 0x00 },
    { 0x4f, 0x00 },
    { 0x5c, 0x00 },
    { 0x5d, 0x00 },
    { 0x5e, 0xae },
    { 0x5f, 0xc3 },
    { 0x60, 0x00 },
    { 0x61, 0x45 },
    { 0x62, 0xa1 },
    { 0x63, 0xcb },
    { 0x64, 0x07 },
    { 0x65, 0x7e },
    { 0x66, 0x72 },
    { 0x67, 0x94 },
    { 0x68, 0xc0 },
    { 0x69, 0x00 },
    { 0x6a, 0x00 },
    { 0x6b, 0x00 },
    { 0x6c, 0x04 },
    { 0x6d, 0xc1 },
    { 0x6e, 0xff },
    { 0x6f, 0x93 },
    { 0x74, 0x00 },
    { 0x75, 0x80 },
    { 0x76, 0x00 },
    { 0x77, 0x00 },
    { 0x00, 0x2d },
    { 0x18, 0x7b },
    { 0x19, 0x3e },
    { 0x1a, 0x00 },
    { 0x1b, 0x6d },
    { 0x1c, 0x00 },
    { 0x1d, 0x00 },
    { 0x1e, 0xae },
    { 0x1f, 0xc3 },
    { 0x20, 0x00 },
    { 0x21, 0x00 },
    { 0x22, 0x00 },
    { 0x23, 0x00 },
    { 0x24, 0x00 },
    { 0x25, 0x00 },
    { 0x26, 0x00 },
    { 0x27, 0x00 },
    { 0x28, 0x00 },
    { 0x29, 0x00 },
    { 0x2a, 0x00 },
    { 0x2b, 0x00 },
    { 0x2c, 0x00 },
    { 0x2d, 0x80 },
    { 0x2e, 0x00 },
    { 0x2f, 0x00 },
    { 0x00, 0x2e },
    { 0x24, 0x20 },
    { 0x25, 0x29 },
    { 0x26, 0x00 },
    { 0x27, 0x94 },
    { 0x00, 0x31 },
    { 0x48, 0x40 },
    { 0x49, 0x00 },
    { 0x4a, 0x00 },
    { 0x4b, 0x00 },
    { 0x4c, 0x00 },
    { 0x4d, 0x00 },
    { 0x4e, 0x00 },
    { 0x4f, 0x00 },
    { 0x50, 0x00 },
    { 0x51, 0x00 },
    { 0x52, 0x00 },
    { 0x53, 0x00 },
    { 0x54, 0x00 },
    { 0x55, 0x00 },
    { 0x56, 0x00 },
    { 0x57, 0x00 },
    { 0x58, 0x00 },
    { 0x59, 0x00 },
    { 0x5a, 0x00 },
    { 0x5b, 0x00 },
    { 0x5c, 0x00 },
    { 0x5d, 0x00 },
    { 0x5e, 0x00 },
    { 0x5f, 0x00 },
    { 0x60, 0x00 },
    { 0x61, 0x00 },
    { 0x62, 0x00 },
    { 0x63, 0x00 },
    { 0x64, 0x00 },
    { 0x65, 0x00 },
    { 0x66, 0x00 },
    { 0x67, 0x00 },
    { 0x68, 0x00 },
    { 0x69, 0x00 },
    { 0x6a, 0x00 },
    { 0x6b, 0x00 },
    { 0x6c, 0x00 },
    { 0x6d, 0x00 },
    { 0x6e, 0x00 },
    { 0x6f, 0x00 },
    { 0x70, 0x00 },
    { 0x71, 0x00 },
    { 0x72, 0x00 },
    { 0x73, 0x00 },
    { 0x74, 0x00 },
    { 0x75, 0x00 },
    { 0x76, 0x00 },
    { 0x77, 0x00 },
    { 0x78, 0x00 },
    { 0x79, 0x00 },
    { 0x7a, 0x00 },
    { 0x7b, 0x00 },
    { 0x7c, 0x00 },
    { 0x7d, 0x00 },
    { 0x7e, 0x00 },
    { 0x7f, 0x00 },
    { 0x00, 0x32 },
    { 0x08, 0x00 },
    { 0x09, 0x00 },
    { 0x0a, 0x00 },
    { 0x0b, 0x00 },
    { 0x0c, 0x00 },
    { 0x0d, 0x00 },
    { 0x0e, 0x00 },
    { 0x0f, 0x00 },
    { 0x10, 0x00 },
    { 0x11, 0x00 },
    { 0x12, 0x00 },
    { 0x13, 0x00 },
    { 0x14, 0x00 },
    { 0x15, 0x00 },
    { 0x16, 0x00 },
    { 0x17, 0x00 },
    { 0x18, 0x00 },
    { 0x19, 0x00 },
    { 0x1a, 0x00 },
    { 0x1b, 0x00 },
    { 0x1c, 0x00 },
    { 0x1d, 0x00 },
    { 0x1e, 0x00 },
    { 0x1f, 0x00 },
    { 0x20, 0x00 },
    { 0x21, 0x00 },
    { 0x22, 0x00 },
    { 0x23, 0x00 },
    { 0x24, 0x00 },
    { 0x25, 0x00 },
    { 0x26, 0x00 },
    { 0x27, 0x00 },
    { 0x28, 0x00 },
    { 0x29, 0x00 },
    { 0x2a, 0x00 },
    { 0x2b, 0x00 },
    { 0x2c, 0x00 },
    { 0x2d, 0x00 },
    { 0x2e, 0x00 },
    { 0x2f, 0x00 },
    { 0x30, 0x00 },
    { 0x31, 0x00 },
    { 0x32, 0x00 },
    { 0x33, 0x00 },
    { 0x34, 0x00 },
    { 0x35, 0x00 },
    { 0x36, 0x00 },
    { 0x37, 0x00 },
    { 0x38, 0x00 },
    { 0x39, 0x00 },
    { 0x3a, 0x00 },
    { 0x3b, 0x00 },
    { 0x3c, 0x00 },
    { 0x3d, 0x00 },
    { 0x3e, 0x00 },
    { 0x3f, 0x00 },
    { 0x40, 0x00 },
    { 0x41, 0x00 },
    { 0x42, 0x00 },
    { 0x43, 0x00 },
    { 0x44, 0x00 },
    { 0x45, 0x00 },
    { 0x46, 0x00 },
    { 0x47, 0x00 },
    { 0x48, 0x00 },
    { 0x49, 0x00 },
    { 0x4a, 0x00 },
    { 0x4b, 0x00 },
    { 0x4c, 0x00 },
    { 0x4d, 0x00 },
    { 0x4e, 0x00 },
    { 0x4f, 0x00 },
    { 0x50, 0x00 },
    { 0x51, 0x00 },
    { 0x52, 0x00 },
    { 0x53, 0x00 },
    { 0x54, 0x00 },
    { 0x55, 0x00 },
    { 0x56, 0x00 },
    { 0x57, 0x00 },
    { 0x58, 0x00 },
    { 0x59, 0x00 },
    { 0x5a, 0x00 },
    { 0x5b, 0x00 },
    { 0x5c, 0x00 },
    { 0x5d, 0x00 },
    { 0x5e, 0x00 },
    { 0x5f, 0x00 },
    { 0x60, 0x00 },
    { 0x61, 0x00 },
    { 0x62, 0x00 },
    { 0x63, 0x00 },
    { 0x64, 0x00 },
    { 0x65, 0x00 },
    { 0x66, 0x00 },
    { 0x67, 0x00 },
    { 0x68, 0x00 },
    { 0x69, 0x00 },
    { 0x6a, 0x00 },
    { 0x6b, 0x00 },
    { 0x6c, 0x00 },
    { 0x6d, 0x00 },
    { 0x6e, 0x00 },
    { 0x6f, 0x00 },
    { 0x70, 0x00 },
    { 0x71, 0x00 },
    { 0x72, 0x00 },
    { 0x73, 0x00 },
    { 0x74, 0x00 },
    { 0x75, 0x00 },
    { 0x76, 0x00 },
    { 0x77, 0x00 },
    { 0x78, 0x00 },
    { 0x79, 0x00 },
    { 0x7a, 0x00 },
    { 0x7b, 0x00 },
    { 0x7c, 0x00 },
    { 0x7d, 0x00 },
    { 0x7e, 0x00 },
    { 0x7f, 0x00 },
    { 0x00, 0x33 },
    { 0x08, 0x00 },
    { 0x09, 0x00 },
    { 0x0a, 0x00 },
    { 0x0b, 0x00 },
    { 0x0c, 0x00 },
    { 0x0d, 0x00 },
    { 0x0e, 0x00 },
    { 0x0f, 0x00 },
    { 0x10, 0x00 },
    { 0x11, 0x00 },
    { 0x12, 0x00 },
    { 0x13, 0x00 },
    { 0x14, 0x00 },
    { 0x15, 0x00 },
    { 0x16, 0x00 },
    { 0x17, 0x00 },
    { 0x18, 0x00 },
    { 0x19, 0x00 },
    { 0x1a, 0x00 },
    { 0x1b, 0x00 },
    { 0x1c, 0x00 },
    { 0x1d, 0x00 },
    { 0x1e, 0x00 },
    { 0x1f, 0x00 },
    { 0x20, 0x00 },
    { 0x21, 0x00 },
    { 0x22, 0x00 },
    { 0x23, 0x00 },
    { 0x24, 0x00 },
    { 0x25, 0x00 },
    { 0x26, 0x00 },
    { 0x27, 0x00 },
    { 0x28, 0x00 },
    { 0x29, 0x00 },
    { 0x2a, 0x00 },
    { 0x2b, 0x00 },
    { 0x2c, 0x00 },
    { 0x2d, 0x00 },
    { 0x2e, 0x00 },
    { 0x2f, 0x00 },
    { 0x30, 0x00 },
    { 0x31, 0x00 },
    { 0x32, 0x00 },
    { 0x33, 0x00 },
    { 0x34, 0x00 },
    { 0x35, 0x00 },
    { 0x36, 0x00 },
    { 0x37, 0x00 },
    { 0x38, 0x00 },
    { 0x39, 0x00 },
    { 0x3a, 0x00 },
    { 0x3b, 0x00 },
    { 0x3c, 0x00 },
    { 0x3d, 0x00 },
    { 0x3e, 0x00 },
    { 0x3f, 0x00 },
    { 0x40, 0x00 },
    { 0x41, 0x00 },
    { 0x42, 0x00 },
    { 0x43, 0x00 },
    { 0x44, 0x00 },
    { 0x45, 0x00 },
    { 0x46, 0x00 },
    { 0x47, 0x00 },
    { 0x48, 0x00 },
    { 0x49, 0x00 },
    { 0x4a, 0x00 },
    { 0x4b, 0x00 },
    { 0x4c, 0x00 },
    { 0x4d, 0x00 },
    { 0x4e, 0x00 },
    { 0x4f, 0x00 },
    { 0x50, 0x00 },
    { 0x51, 0x00 },
    { 0x52, 0x00 },
    { 0x53, 0x00 },
    { 0x54, 0x00 },
    { 0x55, 0x00 },
    { 0x56, 0x00 },
    { 0x57, 0x00 },
    { 0x58, 0x00 },
    { 0x59, 0x00 },
    { 0x5a, 0x00 },
    { 0x5b, 0x00 },
    { 0x5c, 0x00 },
    { 0x5d, 0x00 },
    { 0x5e, 0x00 },
    { 0x5f, 0x00 },
    { 0x60, 0x00 },
    { 0x61, 0x00 },
    { 0x62, 0x00 },
    { 0x63, 0x00 },
    { 0x64, 0x00 },
    { 0x65, 0x00 },
    { 0x66, 0x00 },
    { 0x67, 0x00 },
    { 0x68, 0x00 },
    { 0x69, 0x00 },
    { 0x6a, 0x00 },
    { 0x6b, 0x00 },
    { 0x6c, 0x00 },
    { 0x6d, 0x00 },
    { 0x6e, 0x00 },
    { 0x6f, 0x00 },
    { 0x70, 0x00 },
    { 0x71, 0x00 },
    { 0x72, 0x00 },
    { 0x73, 0x00 },
    { 0x74, 0x00 },
    { 0x75, 0x00 },
    { 0x76, 0x00 },
    { 0x77, 0x00 },
    { 0x78, 0x00 },
    { 0x79, 0x00 },
    { 0x7a, 0x00 },
    { 0x7b, 0x00 },
    { 0x7c, 0x00 },
    { 0x7d, 0x00 },
    { 0x7e, 0x00 },
    { 0x7f, 0x00 },
    { 0x00, 0x34 },
    { 0x08, 0x00 },
    { 0x09, 0x00 },
    { 0x0a, 0x00 },
    { 0x0b, 0x00 },
    { 0x0c, 0x00 },
    { 0x0d, 0x00 },
    { 0x0e, 0x00 },
    { 0x0f, 0x00 },
    { 0x10, 0x00 },
    { 0x11, 0x00 },
    { 0x12, 0x00 },
    { 0x13, 0x00 },
    { 0x14, 0x00 },
    { 0x15, 0x00 },
    { 0x16, 0x00 },
    { 0x17, 0x00 },
    { 0x18, 0x00 },
    { 0x19, 0x00 },
    { 0x1a, 0x00 },
    { 0x1b, 0x00 },
    { 0x1c, 0x00 },
    { 0x1d, 0x00 },
    { 0x1e, 0x00 },
    { 0x1f, 0x00 },
    { 0x20, 0x00 },
    { 0x21, 0x00 },
    { 0x22, 0x00 },
    { 0x23, 0x00 },
    { 0x24, 0x00 },
    { 0x25, 0x00 },
    { 0x26, 0x00 },
    { 0x27, 0x00 },
    { 0x28, 0x00 },
    { 0x29, 0x00 },
    { 0x2a, 0x00 },
    { 0x2b, 0x00 },
    { 0x2c, 0x00 },
    { 0x2d, 0x00 },
    { 0x2e, 0x00 },
    { 0x2f, 0x00 },
    { 0x30, 0x00 },
    { 0x31, 0x00 },
    { 0x32, 0x00 },
    { 0x33, 0x00 },
    { 0x34, 0x00 },
    { 0x35, 0x00 },
    { 0x36, 0x00 },
    { 0x37, 0x00 },
    { 0x38, 0x00 },
    { 0x39, 0x00 },
    { 0x3a, 0x00 },
    { 0x3b, 0x00 },
    { 0x3c, 0x00 },
    { 0x3d, 0x00 },
    { 0x3e, 0x00 },
    { 0x3f, 0x00 },
    { 0x40, 0x00 },
    { 0x41, 0x00 },
    { 0x42, 0x00 },
    { 0x43, 0x00 },
    { 0x44, 0x00 },
    { 0x45, 0x00 },
    { 0x46, 0x00 },
    { 0x47, 0x00 },
    { 0x48, 0x00 },
    { 0x49, 0x00 },
    { 0x4a, 0x00 },
    { 0x4b, 0x00 },
    { 0x4c, 0x00 },
    { 0x4d, 0x00 },
    { 0x4e, 0x00 },
    { 0x4f, 0x00 },
    { 0x50, 0x00 },
    { 0x51, 0x00 },
    { 0x52, 0x00 },
    { 0x53, 0x00 },
    { 0x54, 0x00 },
    { 0x55, 0x00 },
    { 0x56, 0x00 },
    { 0x57, 0x00 },
    { 0x58, 0x00 },
    { 0x59, 0x00 },
    { 0x5a, 0x00 },
    { 0x5b, 0x00 },
    { 0x5c, 0x00 },
    { 0x5d, 0x00 },
    { 0x5e, 0x00 },
    { 0x5f, 0x00 },
    { 0x60, 0x00 },
    { 0x61, 0x00 },
    { 0x62, 0x00 },
    { 0x63, 0x00 },
    { 0x64, 0x00 },
    { 0x65, 0x00 },
    { 0x66, 0x00 },
    { 0x67, 0x00 },
    { 0x68, 0x00 },
    { 0x69, 0x00 },
    { 0x6a, 0x00 },
    { 0x6b, 0x00 },
    { 0x6c, 0x00 },
    { 0x6d, 0x00 },
    { 0x6e, 0x00 },
    { 0x6f, 0x00 },
    { 0x70, 0x00 },
    { 0x71, 0x00 },
    { 0x72, 0x00 },
    { 0x73, 0x00 },
    { 0x74, 0x00 },
    { 0x75, 0x00 },
    { 0x76, 0x00 },
    { 0x77, 0x00 },
    { 0x78, 0x00 },
    { 0x79, 0x00 },
    { 0x7a, 0x00 },
    { 0x7b, 0x00 },
    { 0x7c, 0x00 },
    { 0x7d, 0x00 },
    { 0x7e, 0x00 },
    { 0x7f, 0x00 },
    { 0x00, 0x35 },
    { 0x08, 0x00 },
    { 0x09, 0x00 },
    { 0x0a, 0x00 },
    { 0x0b, 0x00 },
    { 0x0c, 0x00 },
    { 0x0d, 0x00 },
    { 0x0e, 0x00 },
    { 0x0f, 0x00 },
    { 0x10, 0x00 },
    { 0x11, 0x00 },
    { 0x12, 0x00 },
    { 0x13, 0x00 },
    { 0x14, 0x00 },
    { 0x15, 0x00 },
    { 0x16, 0x00 },
    { 0x17, 0x00 },
    { 0x18, 0x00 },
    { 0x19, 0x00 },
    { 0x1a, 0x00 },
    { 0x1b, 0x00 },
    { 0x1c, 0x00 },
    { 0x1d, 0x00 },
    { 0x1e, 0x00 },
    { 0x1f, 0x00 },
    { 0x20, 0x00 },
    { 0x21, 0x00 },
    { 0x22, 0x00 },
    { 0x23, 0x00 },
    { 0x24, 0x00 },
    { 0x25, 0x00 },
    { 0x26, 0x00 },
    { 0x27, 0x00 },
    { 0x28, 0x00 },
    { 0x29, 0x00 },
    { 0x2a, 0x00 },
    { 0x2b, 0x00 },
    { 0x2c, 0x00 },
    { 0x2d, 0x00 },
    { 0x2e, 0x00 },
    { 0x2f, 0x00 },
    { 0x30, 0x00 },
    { 0x31, 0x00 },
    { 0x32, 0x00 },
    { 0x33, 0x00 },
    { 0x34, 0x00 },
    { 0x35, 0x00 },
    { 0x36, 0x00 },
    { 0x37, 0x00 },
    { 0x38, 0x00 },
    { 0x39, 0x00 },
    { 0x3a, 0x00 },
    { 0x3b, 0x00 },
    { 0x3c, 0x00 },
    { 0x3d, 0x00 },
    { 0x3e, 0x00 },
    { 0x3f, 0x00 },
    { 0x40, 0x00 },
    { 0x41, 0x00 },
    { 0x42, 0x00 },
    { 0x43, 0x00 },
    { 0x44, 0x00 },
    { 0x45, 0x00 },
    { 0x46, 0x00 },
    { 0x47, 0x00 },
    { 0x48, 0x00 },
    { 0x49, 0x00 },
    { 0x4a, 0x00 },
    { 0x4b, 0x00 },
    { 0x4c, 0x00 },
    { 0x4d, 0x00 },
    { 0x4e, 0x00 },
    { 0x4f, 0x00 },
    { 0x50, 0x00 },
    { 0x51, 0x00 },
    { 0x52, 0x00 },
    { 0x53, 0x00 },
    { 0x54, 0x00 },
    { 0x55, 0x00 },
    { 0x56, 0x00 },
    { 0x57, 0x00 },
    { 0x58, 0x00 },
    { 0x59, 0x00 },
    { 0x5a, 0x00 },
    { 0x5b, 0x00 },
    { 0x5c, 0x00 },
    { 0x5d, 0x00 },
    { 0x5e, 0x00 },
    { 0x5f, 0x00 },
    { 0x60, 0x00 },
    { 0x61, 0x00 },
    { 0x62, 0x00 },
    { 0x63, 0x00 },
    { 0x64, 0x00 },
    { 0x65, 0x00 },
    { 0x66, 0x00 },
    { 0x67, 0x00 },
    { 0x00, 0x00 },
    { 0x7f, 0xaa },
    { 0x00, 0x24 },
    { 0x18, 0x08 },
    { 0x19, 0x00 },
    { 0x1a, 0x00 },
    { 0x1b, 0x00 },
    { 0x1c, 0x00 },
    { 0x1d, 0x00 },
    { 0x1e, 0x00 },
    { 0x1f, 0x00 },
    { 0x20, 0x00 },
    { 0x21, 0x00 },
    { 0x22, 0x00 },
    { 0x23, 0x00 },
    { 0x24, 0x00 },
    { 0x25, 0x00 },
    { 0x26, 0x00 },
    { 0x27, 0x00 },
    { 0x28, 0x00 },
    { 0x29, 0x00 },
    { 0x2a, 0x00 },
    { 0x2b, 0x00 },
    { 0x2c, 0x08 },
    { 0x2d, 0x00 },
    { 0x2e, 0x00 },
    { 0x2f, 0x00 },
    { 0x30, 0x00 },
    { 0x31, 0x00 },
    { 0x32, 0x00 },
    { 0x33, 0x00 },
    { 0x34, 0x00 },
    { 0x35, 0x00 },
    { 0x36, 0x00 },
    { 0x37, 0x00 },
    { 0x38, 0x00 },
    { 0x39, 0x00 },
    { 0x3a, 0x00 },
    { 0x3b, 0x00 },
    { 0x3c, 0x00 },
    { 0x3d, 0x00 },
    { 0x3e, 0x00 },
    { 0x3f, 0x00 },
    { 0x40, 0x08 },
    { 0x41, 0x00 },
    { 0x42, 0x00 },
    { 0x43, 0x00 },
    { 0x44, 0x00 },
    { 0x45, 0x00 },
    { 0x46, 0x00 },
    { 0x47, 0x00 },
    { 0x48, 0x00 },
    { 0x49, 0x00 },
    { 0x4a, 0x00 },
    { 0x4b, 0x00 },
    { 0x4c, 0x00 },
    { 0x4d, 0x00 },
    { 0x4e, 0x00 },
    { 0x4f, 0x00 },
    { 0x50, 0x00 },
    { 0x51, 0x00 },
    { 0x52, 0x00 },
    { 0x53, 0x00 },
    { 0x54, 0x08 },
    { 0x55, 0x00 },
    { 0x56, 0x00 },
    { 0x57, 0x00 },
    { 0x58, 0x00 },
    { 0x59, 0x00 },
    { 0x5a, 0x00 },
    { 0x5b, 0x00 },
    { 0x5c, 0x00 },
    { 0x5d, 0x00 },
    { 0x5e, 0x00 },
    { 0x5f, 0x00 },
    { 0x60, 0x00 },
    { 0x61, 0x00 },
    { 0x62, 0x00 },
    { 0x63, 0x00 },
    { 0x64, 0x00 },
    { 0x65, 0x00 },
    { 0x66, 0x00 },
    { 0x67, 0x00 },
    { 0x68, 0x08 },
    { 0x69, 0x00 },
    { 0x6a, 0x00 },
    { 0x6b, 0x00 },
    { 0x6c, 0x00 },
    { 0x6d, 0x00 },
    { 0x6e, 0x00 },
    { 0x6f, 0x00 },
    { 0x70, 0x00 },
    { 0x71, 0x00 },
    { 0x72, 0x00 },
    { 0x73, 0x00 },
    { 0x74, 0x00 },
    { 0x75, 0x00 },
    { 0x76, 0x00 },
    { 0x77, 0x00 },
    { 0x78, 0x00 },
    { 0x79, 0x00 },
    { 0x7a, 0x00 },
    { 0x7b, 0x00 },
    { 0x7c, 0x08 },
    { 0x7d, 0x00 },
    { 0x7e, 0x00 },
    { 0x7f, 0x00 },
    { 0x00, 0x25 },
    { 0x08, 0x00 },
    { 0x09, 0x00 },
    { 0x0a, 0x00 },
    { 0x0b, 0x00 },
    { 0x0c, 0x00 },
    { 0x0d, 0x00 },
    { 0x0e, 0x00 },
    { 0x0f, 0x00 },
    { 0x10, 0x00 },
    { 0x11, 0x00 },
    { 0x12, 0x00 },
    { 0x13, 0x00 },
    { 0x14, 0x00 },
    { 0x15, 0x00 },
    { 0x16, 0x00 },
    { 0x17, 0x00 },
    { 0x18, 0x08 },
    { 0x19, 0x00 },
    { 0x1a, 0x00 },
    { 0x1b, 0x00 },
    { 0x1c, 0x00 },
    { 0x1d, 0x00 },
    { 0x1e, 0x00 },
    { 0x1f, 0x00 },
    { 0x20, 0x00 },
    { 0x21, 0x00 },
    { 0x22, 0x00 },
    { 0x23, 0x00 },
    { 0x24, 0x00 },
    { 0x25, 0x00 },
    { 0x26, 0x00 },
    { 0x27, 0x00 },
    { 0x28, 0x00 },
    { 0x29, 0x00 },
    { 0x2a, 0x00 },
    { 0x2b, 0x00 },
    { 0x2c, 0x08 },
    { 0x2d, 0x00 },
    { 0x2e, 0x00 },
    { 0x2f, 0x00 },
    { 0x30, 0x00 },
    { 0x31, 0x00 },
    { 0x32, 0x00 },
    { 0x33, 0x00 },
    { 0x34, 0x00 },
    { 0x35, 0x00 },
    { 0x36, 0x00 },
    { 0x37, 0x00 },
    { 0x38, 0x00 },
    { 0x39, 0x00 },
    { 0x3a, 0x00 },
    { 0x3b, 0x00 },
    { 0x3c, 0x00 },
    { 0x3d, 0x00 },
    { 0x3e, 0x00 },
    { 0x3f, 0x00 },
    { 0x40, 0x08 },
    { 0x41, 0x00 },
    { 0x42, 0x00 },
    { 0x43, 0x00 },
    { 0x44, 0x00 },
    { 0x45, 0x00 },
    { 0x46, 0x00 },
    { 0x47, 0x00 },
    { 0x48, 0x00 },
    { 0x49, 0x00 },
    { 0x4a, 0x00 },
    { 0x4b, 0x00 },
    { 0x4c, 0x00 },
    { 0x4d, 0x00 },
    { 0x4e, 0x00 },
    { 0x4f, 0x00 },
    { 0x50, 0x00 },
    { 0x51, 0x00 },
    { 0x52, 0x00 },
    { 0x53, 0x00 },
    { 0x54, 0x08 },
    { 0x55, 0x00 },
    { 0x56, 0x00 },
    { 0x57, 0x00 },
    { 0x58, 0x00 },
    { 0x59, 0x00 },
    { 0x5a, 0x00 },
    { 0x5b, 0x00 },
    { 0x5c, 0x00 },
    { 0x5d, 0x00 },
    { 0x5e, 0x00 },
    { 0x5f, 0x00 },
    { 0x60, 0x00 },
    { 0x61, 0x00 },
    { 0x62, 0x00 },
    { 0x63, 0x00 },
    { 0x64, 0x00 },
    { 0x65, 0x00 },
    { 0x66, 0x00 },
    { 0x67, 0x00 },
    { 0x68, 0x08 },
    { 0x69, 0x00 },
    { 0x6a, 0x00 },
    { 0x6b, 0x00 },
    { 0x6c, 0x00 },
    { 0x6d, 0x00 },
    { 0x6e, 0x00 },
    { 0x6f, 0x00 },
    { 0x70, 0x00 },
    { 0x71, 0x00 },
    { 0x72, 0x00 },
    { 0x73, 0x00 },
    { 0x74, 0x00 },
    { 0x75, 0x00 },
    { 0x76, 0x00 },
    { 0x77, 0x00 },
    { 0x78, 0x00 },
    { 0x79, 0x00 },
    { 0x7a, 0x00 },
    { 0x7b, 0x00 },
    { 0x7c, 0x08 },
    { 0x7d, 0x00 },
    { 0x7e, 0x00 },
    { 0x7f, 0x00 },
    { 0x00, 0x26 },
    { 0x08, 0x00 },
    { 0x09, 0x00 },
    { 0x0a, 0x00 },
    { 0x0b, 0x00 },
    { 0x0c, 0x00 },
    { 0x0d, 0x00 },
    { 0x0e, 0x00 },
    { 0x0f, 0x00 },
    { 0x10, 0x00 },
    { 0x11, 0x00 },
    { 0x12, 0x00 },
    { 0x13, 0x00 },
    { 0x14, 0x00 },
    { 0x15, 0x00 },
    { 0x16, 0x00 },
    { 0x17, 0x00 },
    { 0x18, 0x08 },
    { 0x19, 0x00 },
    { 0x1a, 0x00 },
    { 0x1b, 0x00 },
    { 0x1c, 0x00 },
    { 0x1d, 0x00 },
    { 0x1e, 0x00 },
    { 0x1f, 0x00 },
    { 0x20, 0x00 },
    { 0x21, 0x00 },
    { 0x22, 0x00 },
    { 0x23, 0x00 },
    { 0x24, 0x00 },
    { 0x25, 0x00 },
    { 0x26, 0x00 },
    { 0x27, 0x00 },
    { 0x28, 0x00 },
    { 0x29, 0x00 },
    { 0x2a, 0x00 },
    { 0x2b, 0x00 },
    { 0x2c, 0x08 },
    { 0x2d, 0x00 },
    { 0x2e, 0x00 },
    { 0x2f, 0x00 },
    { 0x30, 0x00 },
    { 0x31, 0x00 },
    { 0x32, 0x00 },
    { 0x33, 0x00 },
    { 0x34, 0x00 },
    { 0x35, 0x00 },
    { 0x36, 0x00 },
    { 0x37, 0x00 },
    { 0x38, 0x00 },
    { 0x39, 0x00 },
    { 0x3a, 0x00 },
    { 0x3b, 0x00 },
    { 0x3c, 0x00 },
    { 0x3d, 0x00 },
    { 0x3e, 0x00 },
    { 0x3f, 0x00 },
    { 0x40, 0x08 },
    { 0x41, 0x00 },
    { 0x42, 0x00 },
    { 0x43, 0x00 },
    { 0x44, 0x00 },
    { 0x45, 0x00 },
    { 0x46, 0x00 },
    { 0x47, 0x00 },
    { 0x48, 0x00 },
    { 0x49, 0x00 },
    { 0x4a, 0x00 },
    { 0x4b, 0x00 },
    { 0x4c, 0x00 },
    { 0x4d, 0x00 },
    { 0x4e, 0x00 },
    { 0x4f, 0x00 },
    { 0x50, 0x00 },
    { 0x51, 0x00 },
    { 0x52, 0x00 },
    { 0x53, 0x00 },
    { 0x54, 0x08 },
    { 0x55, 0x00 },
    { 0x56, 0x00 },
    { 0x57, 0x00 },
    { 0x58, 0x00 },
    { 0x59, 0x00 },
    { 0x5a, 0x00 },
    { 0x5b, 0x00 },
    { 0x5c, 0x00 },
    { 0x5d, 0x00 },
    { 0x5e, 0x00 },
    { 0x5f, 0x00 },
    { 0x60, 0x00 },
    { 0x61, 0x00 },
    { 0x62, 0x00 },
    { 0x63, 0x00 },
    { 0x64, 0x00 },
    { 0x65, 0x00 },
    { 0x66, 0x00 },
    { 0x67, 0x00 },
    { 0x68, 0x08 },
    { 0x69, 0x00 },
    { 0x6a, 0x00 },
    { 0x6b, 0x00 },
    { 0x6c, 0x00 },
    { 0x6d, 0x00 },
    { 0x6e, 0x00 },
    { 0x6f, 0x00 },
    { 0x70, 0x00 },
    { 0x71, 0x00 },
    { 0x72, 0x00 },
    { 0x73, 0x00 },
    { 0x74, 0x00 },
    { 0x75, 0x00 },
    { 0x76, 0x00 },
    { 0x77, 0x00 },
    { 0x78, 0x00 },
    { 0x79, 0x00 },
    { 0x7a, 0x00 },
    { 0x7b, 0x00 },
    { 0x7c, 0x08 },
    { 0x7d, 0x00 },
    { 0x7e, 0x00 },
    { 0x7f, 0x00 },
    { 0x00, 0x27 },
    { 0x08, 0x00 },
    { 0x09, 0x00 },
    { 0x0a, 0x00 },
    { 0x0b, 0x00 },
    { 0x0c, 0x00 },
    { 0x0d, 0x00 },
    { 0x0e, 0x00 },
    { 0x0f, 0x00 },
    { 0x10, 0x00 },
    { 0x11, 0x00 },
    { 0x12, 0x00 },
    { 0x13, 0x00 },
    { 0x14, 0x00 },
    { 0x15, 0x00 },
    { 0x16, 0x00 },
    { 0x17, 0x00 },
    { 0x18, 0x08 },
    { 0x19, 0x00 },
    { 0x1a, 0x00 },
    { 0x1b, 0x00 },
    { 0x1c, 0x00 },
    { 0x1d, 0x00 },
    { 0x1e, 0x00 },
    { 0x1f, 0x00 },
    { 0x20, 0x00 },
    { 0x21, 0x00 },
    { 0x22, 0x00 },
    { 0x23, 0x00 },
    { 0x24, 0x00 },
    { 0x25, 0x00 },
    { 0x26, 0x00 },
    { 0x27, 0x00 },
    { 0x28, 0x00 },
    { 0x29, 0x00 },
    { 0x2a, 0x00 },
    { 0x2b, 0x00 },
    { 0x2c, 0x08 },
    { 0x2d, 0x00 },
    { 0x2e, 0x00 },
    { 0x2f, 0x00 },
    { 0x30, 0x00 },
    { 0x31, 0x00 },
    { 0x32, 0x00 },
    { 0x33, 0x00 },
    { 0x34, 0x00 },
    { 0x35, 0x00 },
    { 0x36, 0x00 },
    { 0x37, 0x00 },
    { 0x38, 0x00 },
    { 0x39, 0x00 },
    { 0x3a, 0x00 },
    { 0x3b, 0x00 },
    { 0x3c, 0x00 },
    { 0x3d, 0x00 },
    { 0x3e, 0x00 },
    { 0x3f, 0x00 },
    { 0x40, 0x08 },
    { 0x41, 0x00 },
    { 0x42, 0x00 },
    { 0x43, 0x00 },
    { 0x44, 0x00 },
    { 0x45, 0x00 },
    { 0x46, 0x00 },
    { 0x47, 0x00 },
    { 0x48, 0x00 },
    { 0x49, 0x00 },
    { 0x4a, 0x00 },
    { 0x4b, 0x00 },
    { 0x4c, 0x00 },
    { 0x4d, 0x00 },
    { 0x4e, 0x00 },
    { 0x4f, 0x00 },
    { 0x50, 0x00 },
    { 0x51, 0x00 },
    { 0x52, 0x00 },
    { 0x53, 0x00 },
    { 0x54, 0x08 },
    { 0x55, 0x00 },
    { 0x56, 0x00 },
    { 0x57, 0x00 },
    { 0x58, 0x00 },
    { 0x59, 0x00 },
    { 0x5a, 0x00 },
    { 0x5b, 0x00 },
    { 0x5c, 0x00 },
    { 0x5d, 0x00 },
    { 0x5e, 0x00 },
    { 0x5f, 0x00 },
    { 0x60, 0x00 },
    { 0x61, 0x00 },
    { 0x62, 0x00 },
    { 0x63, 0x00 },
    { 0x64, 0x00 },
    { 0x65, 0x00 },
    { 0x66, 0x00 },
    { 0x67, 0x00 },
    { 0x68, 0x08 },
    { 0x69, 0x00 },
    { 0x6a, 0x00 },
    { 0x6b, 0x00 },
    { 0x6c, 0x00 },
    { 0x6d, 0x00 },
    { 0x6e, 0x00 },
    { 0x6f, 0x00 },
    { 0x70, 0x00 },
    { 0x71, 0x00 },
    { 0x72, 0x00 },
    { 0x73, 0x00 },
    { 0x74, 0x00 },
    { 0x75, 0x00 },
    { 0x76, 0x00 },
    { 0x77, 0x00 },
    { 0x78, 0x00 },
    { 0x79, 0x00 },
    { 0x7a, 0x00 },
    { 0x7b, 0x00 },
    { 0x7c, 0x08 },
    { 0x7d, 0x00 },
    { 0x7e, 0x00 },
    { 0x7f, 0x00 },
    { 0x00, 0x28 },
    { 0x08, 0x00 },
    { 0x09, 0x00 },
    { 0x0a, 0x00 },
    { 0x0b, 0x00 },
    { 0x0c, 0x00 },
    { 0x0d, 0x00 },
    { 0x0e, 0x00 },
    { 0x0f, 0x00 },
    { 0x10, 0x00 },
    { 0x11, 0x00 },
    { 0x12, 0x00 },
    { 0x13, 0x00 },
    { 0x14, 0x00 },
    { 0x15, 0x00 },
    { 0x16, 0x00 },
    { 0x17, 0x00 },
    { 0x18, 0x08 },
    { 0x19, 0x00 },
    { 0x1a, 0x00 },
    { 0x1b, 0x00 },
    { 0x1c, 0x00 },
    { 0x1d, 0x00 },
    { 0x1e, 0x00 },
    { 0x1f, 0x00 },
    { 0x20, 0x00 },
    { 0x21, 0x00 },
    { 0x22, 0x00 },
    { 0x23, 0x00 },
    { 0x24, 0x00 },
    { 0x25, 0x00 },
    { 0x26, 0x00 },
    { 0x27, 0x00 },
    { 0x28, 0x00 },
    { 0x29, 0x00 },
    { 0x2a, 0x00 },
    { 0x2b, 0x00 },
    { 0x2c, 0x08 },
    { 0x2d, 0x00 },
    { 0x2e, 0x00 },
    { 0x2f, 0x00 },
    { 0x30, 0x00 },
    { 0x31, 0x00 },
    { 0x32, 0x00 },
    { 0x33, 0x00 },
    { 0x34, 0x00 },
    { 0x35, 0x00 },
    { 0x36, 0x00 },
    { 0x37, 0x00 },
    { 0x38, 0x00 },
    { 0x39, 0x00 },
    { 0x3a, 0x00 },
    { 0x3b, 0x00 },
    { 0x3c, 0x00 },
    { 0x3d, 0x00 },
    { 0x3e, 0x00 },
    { 0x3f, 0x00 },
    { 0x40, 0x08 },
    { 0x41, 0x00 },
    { 0x42, 0x00 },
    { 0x43, 0x00 },
    { 0x44, 0x00 },
    { 0x45, 0x00 },
    { 0x46, 0x00 },
    { 0x47, 0x00 },
    { 0x48, 0x00 },
    { 0x49, 0x00 },
    { 0x4a, 0x00 },
    { 0x4b, 0x00 },
    { 0x4c, 0x00 },
    { 0x4d, 0x00 },
    { 0x4e, 0x00 },
    { 0x4f, 0x00 },
    { 0x50, 0x00 },
    { 0x51, 0x00 },
    { 0x52, 0x00 },
    { 0x53, 0x00 },
    { 0x54, 0x08 },
    { 0x55, 0x00 },
    { 0x56, 0x00 },
    { 0x57, 0x00 },
    { 0x58, 0x00 },
    { 0x59, 0x00 },
    { 0x5a, 0x00 },
    { 0x5b, 0x00 },
    { 0x5c, 0x00 },
    { 0x5d, 0x00 },
    { 0x5e, 0x00 },
    { 0x5f, 0x00 },
    { 0x60, 0x00 },
    { 0x61, 0x00 },
    { 0x62, 0x00 },
    { 0x63, 0x00 },
    { 0x64, 0x00 },
    { 0x65, 0x00 },
    { 0x66, 0x00 },
    { 0x67, 0x00 },
    { 0x68, 0x08 },
    { 0x69, 0x00 },
    { 0x6a, 0x00 },
    { 0x6b, 0x00 },
    { 0x6c, 0x00 },
    { 0x6d, 0x00 },
    { 0x6e, 0x00 },
    { 0x6f, 0x00 },
    { 0x70, 0x00 },
    { 0x71, 0x00 },
    { 0x72, 0x00 },
    { 0x73, 0x00 },
    { 0x74, 0x00 },
    { 0x75, 0x00 },
    { 0x76, 0x00 },
    { 0x77, 0x00 },
    { 0x78, 0x00 },
    { 0x79, 0x00 },
    { 0x7a, 0x00 },
    { 0x7b, 0x00 },
    { 0x7c, 0x08 },
    { 0x7d, 0x00 },
    { 0x7e, 0x00 },
    { 0x7f, 0x00 },
    { 0x00, 0x29 },
    { 0x08, 0x00 },
    { 0x09, 0x00 },
    { 0x0a, 0x00 },
    { 0x0b, 0x00 },
    { 0x0c, 0x00 },
    { 0x0d, 0x00 },
    { 0x0e, 0x00 },
    { 0x0f, 0x00 },
    { 0x10, 0x00 },
    { 0x11, 0x00 },
    { 0x12, 0x00 },
    { 0x13, 0x00 },
    { 0x14, 0x00 },
    { 0x15, 0x00 },
    { 0x16, 0x00 },
    { 0x17, 0x00 },
    { 0x00, 0x2e },
    { 0x7c, 0x08 },
    { 0x7d, 0x00 },
    { 0x7e, 0x00 },
    { 0x7f, 0x00 },
    { 0x00, 0x2f },
    { 0x08, 0x00 },
    { 0x09, 0x00 },
    { 0x0a, 0x00 },
    { 0x0b, 0x00 },
    { 0x0c, 0x00 },
    { 0x0d, 0x00 },
    { 0x0e, 0x00 },
    { 0x0f, 0x00 },
    { 0x10, 0x00 },
    { 0x11, 0x00 },
    { 0x12, 0x00 },
    { 0x13, 0x00 },
    { 0x14, 0x00 },
    { 0x15, 0x00 },
    { 0x16, 0x00 },
    { 0x17, 0x00 },
    { 0x1c, 0x08 },
    { 0x1d, 0x00 },
    { 0x1e, 0x00 },
    { 0x1f, 0x00 },
    { 0x20, 0x00 },
    { 0x21, 0x00 },
    { 0x22, 0x00 },
    { 0x23, 0x00 },
    { 0x24, 0x00 },
    { 0x25, 0x00 },
    { 0x26, 0x00 },
    { 0x27, 0x00 },
    { 0x28, 0x00 },
    { 0x29, 0x00 },
    { 0x2a, 0x00 },
    { 0x2b, 0x00 },
    { 0x2c, 0x00 },
    { 0x2d, 0x00 },
    { 0x2e, 0x00 },
    { 0x2f, 0x00 },
    { 0x00, 0x2a },
    { 0x48, 0x00 },
    { 0x49, 0x0c },
    { 0x4a, 0x4a },
    { 0x4b, 0x50 },
    { 0x4c, 0x00 },
    { 0x4d, 0x0c },
    { 0x4e, 0x4a },
    { 0x4f, 0x50 },
    { 0x50, 0x00 },
    { 0x51, 0x0c },
    { 0x52, 0x4a },
    { 0x53, 0x50 },
    { 0x54, 0x7c },
    { 0x55, 0x72 },
    { 0x56, 0x52 },
    { 0x57, 0x6a },
    { 0x58, 0x86 },
    { 0x59, 0xea },
    { 0x5a, 0x31 },
    { 0x5b, 0xec },
    { 0x00, 0x00 },
    { 0x7f, 0x8c },
    { 0x00, 0x2b },
    { 0x34, 0x00 },
    { 0x35, 0x36 },
    { 0x36, 0x91 },
    { 0x37, 0x5e },
    { 0x38, 0x00 },
    { 0x39, 0x2d },
    { 0x3a, 0x7a },
    { 0x3b, 0xc2 },
    { 0x3c, 0x00 },
    { 0x3d, 0x06 },
    { 0x3e, 0xd3 },
    { 0x3f, 0x72 },
    { 0x40, 0x00 },
    { 0x41, 0x00 },
    { 0x42, 0x00 },
    { 0x43, 0x00 },
    { 0x44, 0xff },
    { 0x45, 0xf0 },
    { 0x46, 0x48 },
    { 0x47, 0x51 },
    { 0x48, 0xff },
    { 0x49, 0x81 },
    { 0x4a, 0x47 },
    { 0x4b, 0xae },
    { 0x4c, 0xfb },
    { 0x4d, 0x83 },
    { 0x4e, 0xf1 },
    { 0x4f, 0x11 },
    { 0x50, 0xfd },
    { 0x51, 0xec },
    { 0x52, 0x7d },
    { 0x53, 0xd4 },
    { 0x54, 0x00 },
    { 0x55, 0x55 },
    { 0x56, 0x0a },
    { 0x57, 0x96 },
    { 0x58, 0x00 },
    { 0x59, 0x09 },
    { 0x5a, 0x55 },
    { 0x5b, 0x52 },
    { 0x00, 0x2d },
    { 0x58, 0x02 },
    { 0x59, 0xa3 },
    { 0x5a, 0x9a },
    { 0x5b, 0xcc },
    { 0x5c, 0x02 },
    { 0x5d, 0xa3 },
    { 0x5e, 0x9a },
    { 0x5f, 0xcc },
    { 0x60, 0x00 },
    { 0x61, 0x06 },
    { 0x62, 0xd3 },
    { 0x63, 0x72 },
    { 0x64, 0x00 },
    { 0x65, 0x01 },
    { 0x66, 0x4a },
    { 0x67, 0xfd },
    { 0x68, 0x00 },
    { 0x69, 0x00 },
    { 0x6a, 0x00 },
    { 0x6b, 0x00 },
    { 0x6c, 0xff },
    { 0x6d, 0x81 },
    { 0x6e, 0x47 },
    { 0x6f, 0xae },
    { 0x70, 0xf9 },
    { 0x71, 0xda },
    { 0x72, 0xbc },
    { 0x73, 0x21 },
    { 0x74, 0xfd },
    { 0x75, 0xec },
    { 0x76, 0x7d },
    { 0x77, 0xd4 },
    { 0x78, 0x00 },
    { 0x79, 0x00 },
    { 0x7a, 0x00 },
    { 0x7b, 0x00 },
    { 0x7c, 0x00 },
    { 0x7d, 0x00 },
    { 0x7e, 0x00 },
    { 0x7f, 0x00 },
    { 0x00, 0x00 },
    { 0x7f, 0xaa },
    { 0x00, 0x2e },
    { 0x40, 0x09 },
    { 0x41, 0x3e },
    { 0x42, 0xdc },
    { 0x43, 0x80 },
    { 0x44, 0x09 },
    { 0x45, 0x3e },
    { 0x46, 0xdc },
    { 0x47, 0x80 },
    { 0x48, 0x09 },
    { 0x49, 0x3e },
    { 0x4a, 0xdc },
    { 0x4b, 0x80 },
    { 0x4c, 0x46 },
    { 0x4d, 0xfd },
    { 0x4e, 0x9a },
    { 0x4f, 0xc2 },
    { 0x50, 0xcd },
    { 0x51, 0x09 },
    { 0x52, 0x58 },
    { 0x53, 0x7c },
    { 0x00, 0x2b },
    { 0x20, 0x50 },
    { 0x21, 0x3c },
    { 0x22, 0x77 },
    { 0x23, 0x42 },
    { 0x24, 0xaf },
    { 0x25, 0xc3 },
    { 0x26, 0x88 },
    { 0x27, 0xbe },
    { 0x28, 0x50 },
    { 0x29, 0x3c },
    { 0x2a, 0x77 },
    { 0x2b, 0x42 },
    { 0x2c, 0x46 },
    { 0x2d, 0xfd },
    { 0x2e, 0x9a },
    { 0x2f, 0xc2 },
    { 0x30, 0xcd },
    { 0x31, 0x09 },
    { 0x32, 0x58 },
    { 0x33, 0x7c },
    { 0x0c, 0x50 },
    { 0x0d, 0x3c },
    { 0x0e, 0x77 },
    { 0x0f, 0x42 },
    { 0x10, 0xaf },
    { 0x11, 0xc3 },
    { 0x12, 0x88 },
    { 0x13, 0xbe },
    { 0x14, 0x50 },
    { 0x15, 0x3c },
    { 0x16, 0x77 },
    { 0x17, 0x42 },
    { 0x18, 0x46 },
    { 0x19, 0xfd },
    { 0x1a, 0x9a },
    { 0x1b, 0xc2 },
    { 0x1c, 0xcd },
    { 0x1d, 0x09 },
    { 0x1e, 0x58 },
    { 0x1f, 0x7c },
    { 0x00, 0x2a },
    { 0x34, 0x00 },
    { 0x35, 0x0c },
    { 0x36, 0x4a },
    { 0x37, 0x50 },
    { 0x38, 0x00 },
    { 0x39, 0x0c },
    { 0x3a, 0x4a },
    { 0x3b, 0x50 },
    { 0x3c, 0x00 },
    { 0x3d, 0x0c },
    { 0x3e, 0x4a },
    { 0x3f, 0x50 },
    { 0x40, 0x7c },
    { 0x41, 0x72 },
    { 0x42, 0x52 },
    { 0x43, 0x6a },
    { 0x44, 0x86 },
    { 0x45, 0xea },
    { 0x46, 0x31 },
    { 0x47, 0xec },
    { 0x00, 0x00 },
    { 0x7f, 0x8c },
    { 0x00, 0x2d },
    { 0x30, 0x02 },
    { 0x31, 0xa3 },
    { 0x32, 0x9a },
    { 0x33, 0xcc },
    { 0x34, 0x02 },
    { 0x35, 0xa3 },
    { 0x36, 0x9a },
    { 0x37, 0xcc },
    { 0x38, 0x00 },
    { 0x39, 0x06 },
    { 0x3a, 0xd3 },
    { 0x3b, 0x72 },
    { 0x3c, 0x00 },
    { 0x3d, 0x01 },
    { 0x3e, 0x4a },
    { 0x3f, 0xfd },
    { 0x40, 0x00 },
    { 0x41, 0x00 },
    { 0x42, 0x00 },
    { 0x43, 0x00 },
    { 0x44, 0xff },
    { 0x45, 0xaa },
    { 0x46, 0xaa },
    { 0x47, 0xab },
    { 0x48, 0xf9 },
    { 0x49, 0xda },
    { 0x4a, 0xbc },
    { 0x4b, 0x21 },
    { 0x4c, 0xfd },
    { 0x4d, 0xec },
    { 0x4e, 0x7d },
    { 0x4f, 0xd4 },
    { 0x50, 0x00 },
    { 0x51, 0x00 },
    { 0x52, 0x00 },
    { 0x53, 0x00 },
    { 0x54, 0x00 },
    { 0x55, 0x00 },
    { 0x56, 0x00 },
    { 0x57, 0x00 },
    { 0x00, 0x00 },
    { 0x7f, 0xaa },
    { 0x00, 0x2a },
    { 0x5c, 0x7c },
    { 0x5d, 0x7e },
    { 0x5e, 0x9c },
    { 0x5f, 0xba },
    { 0x60, 0x83 },
    { 0x61, 0x81 },
    { 0x62, 0x63 },
    { 0x63, 0x46 },
    { 0x64, 0x7c },
    { 0x65, 0x7e },
    { 0x66, 0x9c },
    { 0x67, 0xba },
    { 0x68, 0x7c },
    { 0x69, 0x72 },
    { 0x6a, 0x52 },
    { 0x6b, 0x6a },
    { 0x6c, 0x86 },
    { 0x6d, 0xea },
    { 0x6e, 0x31 },
    { 0x6f, 0xec },
    { 0x70, 0x7c },
    { 0x71, 0x7e },
    { 0x72, 0x9c },
    { 0x73, 0xba },
    { 0x74, 0x83 },
    { 0x75, 0x81 },
    { 0x76, 0x63 },
    { 0x77, 0x46 },
    { 0x78, 0x7c },
    { 0x79, 0x7e },
    { 0x7a, 0x9c },
    { 0x7b, 0xba },
    { 0x7c, 0x7c },
    { 0x7d, 0x72 },
    { 0x7e, 0x52 },
    { 0x7f, 0x6a },
    { 0x00, 0x2b },
    { 0x08, 0x86 },
    { 0x09, 0xea },
    { 0x0a, 0x31 },
    { 0x0b, 0xec },
    { 0x00, 0x2e },
    { 0x54, 0x09 },
    { 0x55, 0x3e },
    { 0x56, 0xdc },
    { 0x57, 0x80 },
    { 0x58, 0x09 },
    { 0x59, 0x3e },
    { 0x5a, 0xdc },
    { 0x5b, 0x80 },
    { 0x5c, 0x09 },
    { 0x5d, 0x3e },
    { 0x5e, 0xdc },
    { 0x5f, 0x80 },
    { 0x60, 0x46 },
    { 0x61, 0xfd },
    { 0x62, 0x9a },
    { 0x63, 0xc2 },
    { 0x64, 0xcd },
    { 0x65, 0x09 },
    { 0x66, 0x58 },
    { 0x67, 0x7c },
    { 0x00, 0x00 },
    { 0x7f, 0x8c },
    { 0x00, 0x2e },
    { 0x10, 0x00 },
    { 0x11, 0x80 },
    { 0x12, 0x00 },
    { 0x13, 0x00 },
    { 0x0c, 0x00 },
    { 0x0d, 0x80 },
    { 0x0e, 0x00 },
    { 0x0f, 0x00 },
    { 0x08, 0x00 },
    { 0x09, 0x80 },
    { 0x0a, 0x00 },
    { 0x0b, 0x00 },
    { 0x18, 0x07 },
    { 0x19, 0xec },
    { 0x1a, 0xa9 },
    { 0x1b, 0xcd },
    { 0x1c, 0x04 },
    { 0x1d, 0x09 },
    { 0x1e, 0xc2 },
    { 0x1f, 0xb1 },
    { 0x20, 0x04 },
    { 0x21, 0x09 },
    { 0x22, 0xc2 },
    { 0x23, 0xb1 },

//Register Tuning
    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x30, 0x00 },
    { 0x4c, 0x30 },
    { 0x03, 0x03 },

    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x78, 0x80 },
};


#endif
