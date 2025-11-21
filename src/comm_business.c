#include "comm_business.h"
#include<pthread.h>

uint8_t test_recv_buff[128];
send_th_t app_send_thread;
recv_th_t app_recv_thread;
pthread_mutex_t app_serial_mutex = PTHREAD_MUTEX_INITIALIZER;//互斥量
pthread_cond_t app_serial_cont = PTHREAD_COND_INITIALIZER;//信号量

static bool interval_ms(uint16_t ms)
{
    static time_t 
}

static void app_send_enter(void *param)
{
    static uint8_t test_state = 0;
    uint8_t *send_buff = 0;
    bool result = 0;
    while(true)
    {
        switch(test_state)
        {
        case 0:
            memset(send_buff,0,LENGTH_SIZE);
            send_buff[0] == (uint8_t)BUFF_HEAD;
            send_buff[1] == (uint8_t)(BUFF_HEAD>>8);
            send_buff[2] == BUFF_TYPE_ADD;
            send_buff[3] == (uint8_t)BUFF_END;
            send_buff[4] == (uint8_t)(BUFF_END>>8);
            break;
        case 1:
            send_buff[0] == (uint8_t)BUFF_HEAD;
            send_buff[1] == (uint8_t)(BUFF_HEAD>>8);
            send_buff[2] == BUFF_TYPE_DE;
            send_buff[3] == (uint8_t)BUFF_END;
            send_buff[4] == (uint8_t)(BUFF_END>>8);
            break;
        case 2:
            send_buff[0] == (uint8_t)BUFF_HEAD;
            send_buff[1] == (uint8_t)(BUFF_HEAD>>8);
            send_buff[2] == BUFF_TYPE_MULT;
            send_buff[3] == (uint8_t)BUFF_END;
            send_buff[4] == (uint8_t)(BUFF_END>>8);
            break;
        }
        result = sp.sp_write(sp.fd,(void *)send_buff,LENGTH_SIZE);
        while(result <= 0)
        {
            result = sp.sp_write(sp.fd,(void *)send_buff,LENGTH_SIZE);
        }
    }
}

static void app_recv_enter(void *param)
{
    while(app_recv_thread.running)
    {
        sp.sp_read(sp.fd,(void*)test_recv_buff,sizeof(test_recv_buff));
        
    }
}

static int receive_state_machine(const uint8_t* recv_buff,ssize_t length)
{
    int result = 0;
    bool half_get = 0;
    int recv_step = 0;
    int offset = 0;
    while(offset < length)
    {
        switch(recv_step)
        {
            case GET_NULL:
                if(recv_buff[offset] == (uint8_t)BUFF_HEAD)
                {
                    half_get = 1;
                }
                else if(recv_buff[offset] == (uint8_t)(BUFF_HEAD>>8))
                {
                    if(half_get)
                    {
                        half_get = 0;
                        recv_step = GET_HEAD;
                    }
                }
                else
                {
                }
                break;
            case GET_HEAD:
                if(recv_buff[offset] > 2 || recv_buff[offset] < 0)
                {
                    recv_step = GET_NULL;
                }
                else
                {
                    result = recv_buff[offset];
                    recv_step = GET_DATA;
                }
                break;
            case GET_DATA:
                if(recv_buff[offset] == (uint8_t)BUFF_END)
                {
                    half_get = 1;
                }
                else if(recv_buff[offset] == (uint8_t)(BUFF_END >> 8))
                {
                    if(half_get)
                    {
                        recv_step = GET_END;
                    }
                }
                else
                {
                    recv_step = GET_NULL;
                }
                break;
            case GET_END:
                offset = length;
                break;
            default:
                offset = length;
                break;
        }
        offset++;
    }
    if(recv_step != GET_END)
    {
        return -1;
    }
    else
    {
        return result;
    }
}

int app_transport_thread_init(void)
{
    int send_rc = 0,recv_rc = 0;
    void *arg;
    send_rc = pthread_create(&(app_send_thread.send_thrc),NULL,app_send_enter,arg);
    if(send_rc != 0)
    {
        printf("Fail to create Send thread\r\n");
        return -1;
    }
    recv_rc = pthread_create(&(app_recv_thread.recv_thrc),NULL,app_recv_enter,arg);
    if(recv_rc != 0)
    {
        printf("Fail to create Recv thread\r\n");
        return -1;
    }
    app_send_thread.running = 1;
    app_recv_thread.running = 1;
    return 0;
    
}

int app_transport_thread_deinit(void)
{

}