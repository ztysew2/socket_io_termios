#include "communication_port.h"

struct termios serial_state = {0};

conn_ops_t  s_ops = {0};

static speed_t to_baud(int baud)
{
    switch(baud)
    {
    case 0:
        return B0;
        break;
    case 50:
        return B50;
        break;
    case 75:
        return B75;
        break;
    case 110:
        return B110;
        break;
    case 134:
        return B134;
        break;
    case 150:
        return B150;
        break;
    case 200:
        return B200;
        break;
    case 300:
        return B300;
        break;
    case 600:
        return B600;
        break;
    case 1200:
        return B1200;
        break;
    case 1800:
        return B1800;
        break;
    case 2400:
        return B2400;
        break;
    case 4800:
        return B4800;
        break;
    case 9600:
        return B9600;
        break;
    case 19200:
        return B19200;
        break;
    case 38400:
        return B38400;
        break;
    case 57600:
        return B57600;
        break;
    case 115200:
        return B115200;
        break;
    default:
        return B115200;
        break;
    }
}

static int tcp_type_find(const char* dev,char* host,size_t host_len,char* port,size_t port_len)
{
    if(!dev || !host  || !port)
        return -1;
    if(strncmp(dev,"tcp://",6) != 0)
    {
        return -1;
    }
    const char* p = dev + 6;
    const char* t = strstr(p,':');
    if(t == NULL)
        return -1;
    int hlen = (int)(t - p);
    if((size_t)hlen + 1 > host_len)
        return -1;
    memset(host,0,host_len);
    memcpy(host,p,hlen);
    host[hlen] = '\0';
    p = t + 1;
    size_t plen = strlen(p);
    if((size_t)plen + 1 > port_len)
        return -1;
    memset(port,0,port_len);
    memcpy(port,p,plen);
    port[plen] = '\0';
    return 0;
}

/*串口终端初始化*/
static int comm_ter_serial_open(io_t* sp, char* filePath, bool if_strctr, int baud)
{
    if(!sp)
        return -1;
    sp->fd = open(filePath,O_RDWR|O_NOCTTY|O_NONBLOCK);

    if(sp->fd < 0)
        goto error;

    struct termios serial_termios;


    if(tcgetattr(sp->fd,&serial_termios) < 0)
        goto error;

    cfmakeraw(&serial_termios);
    if(if_strctr)
        serial_termios.c_iflag |= (IXOFF|IXON);
    else
        serial_termios.c_iflag &= ~(IXON|IXOFF|IXANY);
    serial_termios.c_cflag &= ~(PARENB|CSTOPB|CSIZE);
    serial_termios.c_cflag |= CS8;              // 8位
    serial_termios.c_cflag &= ~CRTSCTS;         // 关闭 RTS/CTS 硬件流控
    serial_termios.c_cflag |= CLOCAL|CREAD;     //不抢占控制终端+允许接收

    serial_termios.c_cc[VTIME] = 0;
    serial_termios.c_cc[VMIN] = 0;

    speed_t baud_rate = to_baud(baud);

    if(cfsetospeed(&serial_termios,baud_rate) < 0)
        goto error;
    if(cfsetispeed(&serial_termios,baud_rate) < 0)
        goto error;
    if(tcsetattr(sp->fd,TCSANOW,&serial_termios) < 0)
        goto error;
    if(tcflush(sp->fd,TCIOFLUSH) < 0)
        goto error;
    return 0;
    error:
        if(sp->fd >= 0)
            close(sp->fd);
        sp->fd = -1;
        return -1;

}

/*TCP终端初始化*/
static int comm_ter_socket_open(io_t *sp , char* host_or_ip, char* port_or_service)
{
    if(!sp || !host_or_ip || !port_or_service)
        return -1;
    int result = -1;
    struct addrinfo type_hint;
    struct addrinfo* addr_hint = NULL;
    memset(&type_hint,0,sizeof(type_hint));
    type_hint.ai_addr = NULL;
    type_hint.ai_next = NULL;
    type_hint.ai_family = AF_INET;
    type_hint.ai_socktype = SOCK_STREAM;
    result = getaddrinfo(host_or_ip,port_or_service,&type_hint,&addr_hint);
    if(result != 0)            //"192.169.1.123" "2698"
    {
        fprintf(stderr,"getaddrinfo:%s",gai_strerror(result));
        goto error;
    }
    for(struct addrinfo* list = addr_hint;list != NULL;list=list->ai_next)
    {
        sp->fd = socket(list->ai_family,list->ai_socktype,list->ai_protocol);
        if(sp->fd < 0)
        {
            if(list->ai_next == NULL)
            {
                printf("ALL socket init failed\n");
                goto error;
            }
            continue;
        }
        if(connect(sp->fd,list->ai_addr,list->ai_addrlen) < 0)
        {
            if(list->ai_next == NULL)
            {
                printf("No connect create\n");
                goto error;
            }
            close(sp->fd);
            continue;
        }
        break;
    }
    freeaddrinfo(addr_hint);
    return 0;
    error:
        if(sp->fd >= 0)
            close(sp->fd);
        if(addr_hint != NULL)
            freeaddrinfo(addr_hint);
        return -1;
}

static int comm_ter_open(io_t* sp,
     const char* dev,int baud,int strctr)
{
    if(!sp || !dev)
        return -1;
    char host[128];
    char port[32];
    if(tcp_type_find(dev,host,sizeof(host),port,sizeof(port)) < 0)
    {    
        sp->is_socket = 0;
        if(comm_ter_serial_open(sp,dev,(bool)strctr,baud) < 0)
            return -1;
    }
    else
    {
        sp->is_socket = 1;
        if(comm_ter_socket_open(sp,host,port) < 0)
            return -1;
    }
    
    return 0;
}

static int comm_ter_close(io_t* sp)
{   
    if(!sp)
        return -1;
    if(sp->fd >= 0)
    {
        close(sp->fd);
    }

    sp->fd = -1;
    return 0;
}

static ssize_t comm_ter_write(io_t* sp,const uint8_t* buf,size_t cap)
{
    if(!sp)
        return -1;
    ssize_t iswrite = 0;
    ssize_t n = 0;
    while(iswrite < cap)
    {
        n = write(sp->fd,buf+iswrite,cap-iswrite);
        if(n > 0)
        {
            iswrite += n;
            continue;
        }
        else if(n == 0)
        {
            errno = EIO;
            return -1;
        }
        else
        {
            if(errno == EINTR)
                continue;
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                return 0;
            return -1;
        }
    }
    return iswrite;
}

static ssize_t comm_ter_read(io_t* sp,uint8_t* out,size_t cap)
{
    ssize_t n = 0;
    for(;;)
    {
        n = read(sp->fd,out,cap);
        if(n > 0)
            return n;
        if(n == 0)
            return 0;
        if(errno == EINTR)
            continue;
        if(errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        return -1;
    }
}

io_t* comm_ter_registration(void)
{
    io_t *p = calloc(1,sizeof(*p));
    p->fd = -1;
    p->is_socket = 0;
    if(!p->fd_ops.sp_close || !p->fd_ops.sp_open
    || !p->fd_ops.sp_read || !p->fd_ops.sp_write)
    {
        p->fd_ops.sp_open = comm_ter_open;
        p->fd_ops.sp_read = comm_ter_read;
        p->fd_ops.sp_write = comm_ter_write;
        p->fd_ops.sp_close = comm_ter_close;
    }
    return p;
}