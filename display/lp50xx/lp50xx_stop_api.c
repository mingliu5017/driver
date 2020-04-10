/* Hello World program */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define LED_MAGIC_BASE	 'I'
#define LED_COLSE_TIME_CMD   _IOR(LED_MAGIC_BASE,0x01,int)

int main(int argc, char *argv[])
{

    int fd;
    unsigned int data=0;

	fd = open("/dev/lp50xx", 777);
	if (fd < 0) {
	   perror("open");
	   exit(-2);
	}

	ioctl(fd, LED_COLSE_TIME_CMD, &data);

	close(fd);

    return 0;
}
