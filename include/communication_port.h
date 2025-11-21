#ifndef __COMM_PORT_H__

#define __COMM_PORT_H__
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
#include<asm-generic/termbits.h>
#include<limits.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<netdb.h>

#ifndef LOG_ENABLE
#define LOG_ENABLE  1
#endif

#define EPOLLFD_MAXMUM  10 

typedef struct serial_port 
{
    int fd;
    int (*sp_open)(int* fd ,const char* port_name ,int baud,bool rtcstr);
    ssize_t (*sp_write)(const int fd, const void* buff,size_t len);
    ssize_t (*sp_read)(const int fd, void* buff, size_t len);
    int (*sp_close)(int *sp_fd);
}serial_port_t;

extern serial_port_t sp;

int app_serial_init(serial_port_t *app_serial, const char* io_path, int baud, bool rtcstr);

void app_sleep_ms(uint32_t ms);

void errExit(const char*fmt,...);

#endif


