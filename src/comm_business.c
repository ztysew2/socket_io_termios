#include "comm_business.h"
#include <pthread.h>

struct epoll_event ev_list[EV_MAX] = {0};

uint8_t io_rc_buff[BUFF_SIZE] = {0};

//==================模块==========================================
static size_t app_ringbuff_usedlen(app_ringbuff_t* serial_ringbuff)
{
    if(serial_ringbuff->tail > serial_ringbuff->head)
    {
        return serial_ringbuff->buff_len - (serial_ringbuff->tail-serial_ringbuff->head);
    }
    else
    {
        return serial_ringbuff->head - serial_ringbuff->tail;
    }

}

static int app_ringbuff_init(app_ringbuff_t* serial_ringbuff)
{
    if(!serial_ringbuff)
        return -1;
    if(serial_ringbuff->recv_buff )
    {
        free(serial_ringbuff->recv_buff);
        serial_ringbuff->recv_buff = NULL;
    }
    serial_ringbuff->recv_buff = calloc(RCV_LEN,sizeof(uint8_t));
    if(!serial_ringbuff->recv_buff)
        return -1;
    serial_ringbuff->buff_len = RCV_LEN;
    serial_ringbuff->head = 0;
    serial_ringbuff->tail = 0;
    return 0;
}

static int app_ringbuff_read(app_ringbuff_t* serial_ringbuff,uint8_t* read_buff,size_t len)
{
    if(!serial_ringbuff->recv_buff)
        return -2;
    if(!read_buff)
        return -1;
    size_t had_write = app_ringbuff_usedlen(serial_ringbuff);

    if(len > had_write)
        return -1;
    for(int i = 0; i < len ; i++)
    {
        read_buff[i] = serial_ringbuff->recv_buff[(serial_ringbuff->tail+i) % serial_ringbuff->buff_len];
    }
    serial_ringbuff->tail = (serial_ringbuff->tail + len) % serial_ringbuff->buff_len;
    return 0;
}

static int app_ringbuff_write(app_ringbuff_t* serial_ringbuff,const uint8_t* write_buff,size_t len)
{

    if(!serial_ringbuff->recv_buff || !write_buff)
        return -1;

    size_t used = app_ringbuff_usedlen(serial_ringbuff);
    size_t free = serial_ringbuff->buff_len - used - 1;   // 留一格作为区分空/满

    if (len > free)
        return -1;

    for(int i = 0; i < len ; i++)
    {
        serial_ringbuff->recv_buff[(serial_ringbuff->head + i) % serial_ringbuff->buff_len] = write_buff[i];
    }
    serial_ringbuff->head = (serial_ringbuff->head + len) % (serial_ringbuff->buff_len);
    return 0;
}

static int app_ringbuff_deinit(app_ringbuff_t* serial_ringbuff)
{
    if(!serial_ringbuff)
        return -1;
    if(serial_ringbuff->recv_buff)
    {
        free(serial_ringbuff->recv_buff);
        serial_ringbuff->recv_buff = NULL;
    }
    serial_ringbuff->buff_len = 0;
    serial_ringbuff->head = 0;
    serial_ringbuff->tail = 0;
    return 0;
}

static void txq_init(tx_queue_t* q)
{
    q->head = q->tail = NULL;
    q->queued_bytes = 0;
}

static int txq_push(tx_queue_t* q,const uint8_t* data,size_t len)
{
    if(!q || !data)
        return -1;
    
    tx_msg_t* msg = (tx_msg_t*)malloc(sizeof(tx_msg_t));
    memset(msg->buf,0,len);
    memcpy(msg->buf,data,len);
    msg->len = len;
    msg->sent = 0;
    msg->next = NULL;
    if(!q->tail)
    {
        q->head = q->tail = msg;
    }
    else
    {
        q->tail->next = msg;
        q->tail = q->tail->next;
    }
    q->queued_bytes +=len;
    return 0;
}

static int txq_pop(tx_queue_t* q)
{
    if(!q || !q->head)
        return -1;
    tx_msg_t* p = q->head;
    q->head = q->head->next;
    if(!q->head)
    {
        q->tail = NULL;
    }
    q->queued_bytes -= p->len;
    free(p->buf);
    free(p);
    return 0;
}

static int txq_flush(conn_ctx_t* sp,tx_queue_t* q)
{
    size_t rc = 0;
    if(!q)
        return -1;
    if(!q->head || !q->tail)
        return -1;
    if((q->head == q->tail) && q->queued_bytes == 0)
    {
        return 0;
    }

    tx_msg_t* msg = q->head;
    while(q->head)
    {
        if(msg->len == msg->sent)
        {
            txq_pop(q);
            msg = q->head;
            continue;
        }
        if(!sp->io_data.fd_ops.sp_write)
        {
            return -1;
        }
        rc = sp->io_data.fd_ops.sp_write(&(sp->io_data),msg->buf + msg->sent,msg->len - msg->sent);
        if(rc < 0)
        {
            return -1;
        }
        msg->sent += rc;
    }
    return 0;
    
}

static int txq_deinit(tx_queue_t* q)
{
    if(!q)
        return -1;
    tx_msg_t* temp = q->head;
    while(q->head)
    {   
        q->head = q->head->next;
        free(temp->buf);
        free(temp);
        temp = q->head;
    }

    q->queued_bytes = 0;
    return 0;
}
//=============函数======================
/*函数收发*/

/*io init*/
int Communication_Carrier_Init(conn_ctx_t* sp)
{
    if(!sp)
        return -1;
    if(app_ringbuff_init(&(sp->rxb)) < 0)
        return -1;
    txq_init(&(sp->txq));
    return 0;
}

int Communication_Carrier_Deinit(conn_ctx_t* sp)
{
    if(!sp)
        return -1;
    if(app_ringbuff_deinit(&(sp->rxb)) < 0)
        return -1;
    txq_deinit(&(sp->txq));
    return 0;
}

int io_ctl_init(void)
{
    int epfd = -1;
    epfd = epoll_create(EV_MAX);
    if(epfd < 0)
        return -1;
    else
        return epfd;
}

int io_ctl_act(int epfd,conn_ctx_t* sp,int action)
{
    if(!sp)
        return -1;
    if(epfd < 0 || sp->io_data.fd < 0)
        return -1;
    if(action == EPOLL_CTL_DEL)
    {
        if(epoll_ctl(epfd,action,sp->io_data.fd,NULL) < 0)
            return -1;
        return 0;
    }
    struct epoll_event ev;
    memset(&ev,0,sizeof(ev));
    if(action == EPOLL_CTL_ADD)
    {
        
        ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
        ev.data.ptr = (void*) sp;
        if(epoll_ctl(epfd,action,sp->io_data.fd,&ev) < 0)
        {
            return -1;
        }
    }
    
    return 0;
}

int io_event_orch(int epfd)
{
    int rc;
    if(epfd <= 0)
        return -1;
    rc = epoll_wait(epfd,ev_list,EV_MAX,-1);
    if(rc < 0)
        return -1;
    for(int i = 0 ; i < rc ; i++)
    {
        conn_ctx_t* io_ptr = (conn_ctx_t*)ev_list[i].data.ptr;
        uint32_t io_event = ev_list[i].events;
        if(io_event & EPOLLIN)
        {
            size_t rc = io_ptr->io_data.fd_ops.sp_read(&io_ptr->io_data,io_rc_buff,BUFF_SIZE);
            if(rc < 0)
                goto fd_error;
            if(app_ringbuff_write(&(io_ptr->rxb),io_rc_buff,rc) < -1)
                goto fd_error;
            continue;
        }
        if(io_event & EPOLLOUT)
        {
            if(txq_flush(io_ptr,&(io_ptr->txq)) < 0)
                goto fd_error;
            continue;

        }
        if(io_event & (EPOLLERR|EPOLLHUP))
        {
            goto fd_error;
        }
        fd_error:
            io_ptr->io_data.fd_ops.sp_close(&io_ptr->io_data);
            io_ctl_act(epfd,io_ptr,EV_DEL);
            Communication_Carrier_Deinit(io_ptr);
            continue;
    }
    return 0;
}

