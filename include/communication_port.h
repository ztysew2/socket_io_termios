#ifndef __COMM_PORT_H__

#define __COMM_PORT_H__

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include<stdio.h>
#include<stdint.h>
#include<stddef.h>
#include<errno.h>
#include<stdlib.h>
#include<stdarg.h>
#include<string.h>
#include<time.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdbool.h>
#include<termios.h>
#include<sys/epoll.h>
#include<limits.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "comm_business.h"

#ifndef LOG_ENABLE
#define LOG_ENABLE  1
#endif

#define EPOLLFD_MAXMUM  10 



typedef struct serial_port 
{
    int fd;
    bool is_socket;
    app_ringbuff_t rxb;
    tx_queue_t txq;
    pthread_mutex_t mu;
}io_t;

typedef struct ops
{
    int (*sp_open)(int* fd ,const char* port_name ,int baud,bool rtcstr);
    ssize_t (*sp_write)(const int fd, const void* buff,size_t len);
    ssize_t (*sp_read)(const int fd, void* buff, size_t len);
    int (*sp_close)(int *sp_fd);
}conn_ops_t;

extern conn_ops_t s_ops;

void errExit(const char*fmt,...);

#endif


