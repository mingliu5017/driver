#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <net/if.h> 


int get_local_MAC_Addr(const char *interfaceName, char *mac) 
{ 
    struct ifreq  ifreq; 
    int   sock;  

    if(NULL == interfaceName || NULL == mac){  
        printf("Invalid argument");  
        return -1;  
    }
	
    if((sock=socket(AF_INET,SOCK_STREAM,0)) <0) 
    { 
        perror( "socket "); 
        return -1; 
    } 

	strncpy(ifreq.ifr_name, interfaceName, sizeof(ifreq.ifr_name));
    if(ioctl(sock,SIOCGIFHWADDR,&ifreq) <0) 
    { 
        perror( "ioctl "); 
        return -1; 
    }

    sprintf(mac,"%02x:%02x:%02x:%02x:%02x:%02x",(unsigned   char)ifreq.ifr_hwaddr.sa_data[0], 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[1], 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[2], 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[3], 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[4], 
            (unsigned   char)ifreq.ifr_hwaddr.sa_data[5]);

	return 0; 
} 

