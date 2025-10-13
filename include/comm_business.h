#ifndef __COMM_BUS_H
#define __COMM_BUS_H
#include<communication_port.h>

#define BUFF_HEAD       0x5AA5
#define BUFF_TYPE_ADD   0x00
#define BUFF_TYPE_DE    0x01
#define BUFF_TYPE_MULT  0x02    
#define BUFF_END        0xA55A

#define GET_NULL        0
#define GET_HEAD        1
#define GET_DATA        2
#define GET_END         3

typedef struct send_th
{
    pthread_t send_thrc;
    int running;
}send_th_t;

typedef struct recv_th
{
    pthread_t recv_thrc;
    int running;
}recv_th_t;

#endif