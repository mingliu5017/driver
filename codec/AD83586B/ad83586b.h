#ifndef _AD83586B_H
#define _AD83586B_H

#define MVOL                             0x03
#define C1VOL                            0x04
#define C2VOL                            0x05

#define CFADDR                           0x14
#define A1CF1                            0x15
#define A1CF2                            0x16
#define A1CF3                            0x17
#define CFUD                             0x24

#define AD83586B_REGISTER_COUNT		 			 37
#define AD83586B_RAM_TABLE_COUNT         127

#define I2C_RETRY_DELAY 5 /* ms */  
#define I2C_RETRIES 3
struct ad83586b_platform_data {
	int reset_pin; 	//pd
	int reset_pin2;	//reset
	int power_down_pin;
};

#endif
