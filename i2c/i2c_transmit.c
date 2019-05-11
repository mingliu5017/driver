#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#define I2C_FILE_NAME "/dev/i2c-1"
#define I2C_SLAVE_ADDR 0x48

static int i2c_fd = 0;

static int i2c_read(int file,
                            unsigned short addr,
                            unsigned char *val,
				            unsigned int len) {
    unsigned char outbuf;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];
 
    /* The data will get returned in this structure */
    messages[0].addr  = addr;
    messages[0].flags = I2C_M_RD;
    messages[0].len   = len;
    messages[0].buf   = val;
 
    /* Send the request to the kernel and get the result back */
    packets.msgs      = messages;
    packets.nmsgs     = 1;
    if(ioctl(file, I2C_RDWR, &packets) < 0) {
        perror("i2c_read Unable to send data");
        return -1;
    }
 
    return 0;
}

void I2C_RCV(unsigned char *rxbuffer, unsigned int rxbuffersize){
	if(i2c_read(i2c_fd, I2C_SLAVE_ADDR, rxbuffer, rxbuffersize) < 0){
		printf("I2C recive data faild!\r\n");
	}

	return;
}

static int get_i2c_register(int file,
                            unsigned char addr,
                            unsigned char reg,
                            unsigned char *val) {
    unsigned char inbuf, outbuf;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];

    /*
     * In order to read a register, we first do a "dummy write" by writing
     * 0 bytes to the register we want to read from.  This is similar to
     * the packet in set_i2c_register, except it's 1 byte rather than 2.
     */
    outbuf = reg;
    messages[0].addr  = addr;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = &outbuf;

    /* The data will get returned in this structure */
    messages[1].addr  = addr;
    messages[1].flags = I2C_M_RD/* | I2C_M_NOSTART*/;
    messages[1].len   = sizeof(inbuf);
    messages[1].buf   = &inbuf;

    /* Send the request to the kernel and get the result back */
    packets.msgs      = messages;
    packets.nmsgs     = 2;
    if(ioctl(file, I2C_RDWR, &packets) < 0) {
        perror("Unable to send data");
        return 1;
    }
    *val = inbuf;

    return 0;
}


static int i2c_write(int file,
							unsigned char addr,
							unsigned char *value,
					        int len) {
 	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[1];
	unsigned char *outbuf = (unsigned char *)malloc(sizeof(unsigned char)*(len));

	if(outbuf==NULL)
	{
		perror("MALLOC");
		return -1;
	}
	
	messages[0].addr  = addr;
	messages[0].flags = 0;
	messages[0].len   = len;	
	messages[0].buf   = outbuf;
 
	/* 
	 * The second byte indicates the value to write.  Note that for many
	 * devices, we can write multiple, sequential registers at once by
	 * simply making outbuf bigger.
	 */
    memcpy(outbuf, value, len);
 
	/* Transfer the i2c packets to the kernel and verify it worked */
	packets.msgs  = messages;
	packets.nmsgs = 1;
	if(ioctl(file, I2C_RDWR, &packets) < 0) {
		perror("set_i2c_register Unable to send data");
		return -1;
	}

    free(outbuf);

	return 0;
}

void I2C_Transmit(unsigned char *txbuffer,unsigned int txbuffersize){
	if(i2c_write(i2c_fd, I2C_SLAVE_ADDR, txbuffer, txbuffersize) < 0){
		printf("I2C recive data faild!\r\n");
	}

	return;
}

static int set_i2c_register(int file,
                            unsigned char addr,
                            unsigned char reg,
                            unsigned char value) {

    unsigned char outbuf[2];
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];

    messages[0].addr  = addr;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = outbuf;

    /* The first byte indicates which register we'll write */
    outbuf[0] = reg;

    /*
     * The second byte indicates the value to write.  Note that for many
     * devices, we can write multiple, sequential registers at once by
     * simply making outbuf bigger.
     */
    outbuf[1] = value;

    /* Transfer the i2c packets to the kernel and verify it worked */
    packets.msgs  = messages;
    packets.nmsgs = 1;
    if(ioctl(file, I2C_RDWR, &packets) < 0) {
        perror("Unable to send data");
        return 1;
    }

    return 0;
}

int I2C_open(void){
	int ret = 0;

	i2c_fd = open(I2C_FILE_NAME, O_RDWR);
	if(i2c_fd < 0){
		perror("Unable to open i2c control file!");
		return -1;
	}
	ret = ioctl(i2c_fd, I2C_TIMEOUT, 4); //超时时间
	if(ret < 0) {
		perror("I2C_RDWR ioctl with error.\n");
	}
	ret = ioctl(i2c_fd, I2C_RETRIES, 2); //重复次数
	if(ret < 0) {
		perror("I2C_RDWR ioctl with error code.\n");
	}

	return ret;
}

void I2C_close(void){
	if(i2c_fd > 0) {
		close(i2c_fd);
	}
}

