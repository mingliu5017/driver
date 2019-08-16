#ifndef _AD85050_H
#define _AD85050_H

#define MVOL                             0x03
#define C1VOL                            0x04
#define C2VOL                            0x05

#define CFADDR                           0x1d
#define A1CF1                            0x1e
#define A1CF2                            0x1f
#define A1CF3                            0x20
#define CFUD                             0x2d

#define AD85050_REGISTER_COUNT		     156
#define AD85050_RAM_TABLE_COUNT          180

#define I2C_RETRY_DELAY 5 /* ms */
#define I2C_RETRIES 3
struct ad85050_platform_data {
    //int reset_pin;
};

#endif
