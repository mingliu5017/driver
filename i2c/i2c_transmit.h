#ifndef __I2C_TRANSMIT_H
#define __I2C_TRANSMIT_H

#ifdef __cplusplus
extern "C" {
#endif

void I2C_RCV(unsigned char *rxbuffer, unsigned int rxbuffersize);
void I2C_Transmit(unsigned char *txbuffer,unsigned int txbuffersize);
int I2C_open(void);
void I2C_close(void);

#ifdef __cplusplus
}
#endif

#endif


