#ifndef _HFP_CTL_SOCKET_H
#define _HFP_CTL_SOCKET_H

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <sys/stat.h>

typedef struct{
    int server_sockfd;
    int client_sockfd;
    int server_len;
    int client_len;
    struct sockaddr_un server_address;
    struct sockaddr_un client_address;
    char sock_path[512];
} tAPP_SOCKET;



/********************SERVER API***************************/

int setup_socket_server(tAPP_SOCKET *app_socket);
int accpet_client(tAPP_SOCKET *app_socket);
void teardown_socket_server(tAPP_SOCKET *app_socket);

/********************CLIENT API***************************/
int setup_socket_client(char *socket_path);
void teardown_socket_client(int sockfd);
/********************COMMON API***************************/
int socket_send(int sockfd, char *msg, int len);
int socket_recieve(int sockfd, char *msg, int len);
#endif



