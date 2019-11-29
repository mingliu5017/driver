#ifndef _TAS5805_H
#define _TAS5805_H

#define TAS5805_I2C_ADDR                                0x58

#define TAS5805_PAGE_SEL	                            0x00
#define TAS5805_RESET_CTRL                             	0x01
#define TAS5805_DEVICE_CTRL_1                          	0x02
#define TAS5805_DEVICE_CTRL_2                          	0x03
#define TAS5805_I2C_PAGE_AUTO_INC                      	0x0F
#define TAS5805_SIG_CH_CTRL								0x28 
#define TAS5805_CLOCK_DET_CTRL							0x29
#define TAS5805_CLOCK_ERR_CTRL							0x2A
#define TAS5805_SDOUT_SEL								0x30
#define TAS5805_I2S_CTRL								0x31
#define TAS5805_SAP_CTRL1								0x33
#define TAS5805_SAP_CTRL2								0x34
#define TAS5805_SAP_CTRL3								0x35
#define TAS5805_LSH_DIS									0x36
#define TAS5805_FS_MON									0x37
#define TAS5805_BCK_MON									0x38
#define TAS5805_CLKDET_STATUS							0x39
#define TAS5805_DSP_PGM_MODE							0x40
#define TAS5805_DSP_CTRL Register						0x46
#define TAS5805_DIG_VOL_CTRL1							0x4B
#define TAS5805_DIG_VOL_LEFT							0x4C
#define TAS5805_DIG_VOL_RIGHT							0x4D
#define TAS5805_DIG_VOL_CTRL2							0x4E
#define TAS5805_DIG_VOL_CTRL3							0x4F
#define TAS5805_AUTO_MUTE_CTRL							0x50
#define TAS5805_AUTO_MUTE_TIME							0x51
#define TAS5805_ANA_CTRL								0x53
#define TAS5805_AGAIN									0x54
#define TAS5805_LOW_EMI_MODE							0x55
#define TAS5805_BQ_WR_CTRL1								0x5C
#define TAS5805_DAC_CTRL								0x5D
#define TAS5805_ANA_RAMP_CTRL							0x5F
#define TAS5805_ADR_PIN_CTRL							0x60
#define TAS5805_ADR_PIN_CONFIG							0x61
#define TAS5805_DSP_MISC								0x66
#define TAS5805_DIE_ID									0x67
#define TAS5805_POWER_STATE								0x68
#define TAS5805_AUTOMUTE_STATE							0x69
#define TAS5805_PHASE_CTRL								0x6A
#define TAS5805_SS_CTRL0								0x6B
#define TAS5805_SS_CTRL1								0x6C
#define TAS5805_SS_CTRL2								0x6D
#define TAS5805_SS_CTRL3								0x6E
#define TAS5805_SS_CTRL4								0x6F
#define TAS5805_CHAN_FAULT								0x70
#define TAS5805_GLOBAL_FAULT1							0x71
#define TAS5805_GLOBAL_FAULT2							0x72
#define TAS5805_OT WARNING								0x73
#define TAS5805_PIN_CONTROL1							0x74
#define TAS5805_PIN_CONTROL2							0x75
#define TAS5805_FAULT_CLEAR								0x78
#define TAS5805_BOOK_SEL								0x7F

#define TAS5805_REGISTER_COUNT							0xFF
#endif
