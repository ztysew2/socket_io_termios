#ifndef __COMM_BUS_H
#define __COMM_BUS_H
#include<communication_port.h>

#define BUFF_HEAD       0x5AA5
#define BUFF_TYPE_ADD   0x00
#define BUFF_TYPE_DE    0x01
#define BUFF_TYPE_MULT  0x02    
#define BUFF_END        0xA55A
#define LENGTH_SIZE     5

#define GET_NULL        0
#define GET_HEAD        1
#define GET_DATA        2
#define GET_END         3

#define SND_LEN         1024
#define RCV_LEN         1024   

typedef struct app_ring_buff
{
    uint8_t* recv_buff;
    size_t buff_len;
    size_t head;        //下一次写入
    size_t tail;        //下一次读取
}app_ringbuff_t;

typedef struct tx_msg
{
    uint8_t* buf;//完整整
    size_t len;
    size_t sent;//已发送字节
    struct tx_msg *next;
}tx_msg_t;

typedef struct 
{
    tx_msg_t* head;//队列发送起始
    tx_msg_t* tail;//队列结尾
    size_t    queued_bytes;//当前队列里的字节数
}tx_queue_t;

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