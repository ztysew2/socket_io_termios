#include "communication_port.h"

serial_port_t sp = {0};
struct termios serial_state = {0};

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

static int p_set_termios(int* fd, int baud , bool rctstr)
{
    struct termios tio;
    speed_t io_baud;
    memset(&tio,0,sizeof(tio));

    if(tcgetattr(*fd,&tio) < 0)
    {
        return -1;
    }
    cfmakeraw(&tio);
    //实现8IN1设置
    tio.c_cflag &= ~(CSIZE | PARENB | CSTOPB); 
    tio.c_cflag |= (CS8 | CREAD | CLOCAL); 
    //判断是否启用硬件流量控制
    if(rctstr)
        tio.c_cflag |= CRTSCTS; 
    else
        tio.c_cflag &= ~CRTSCTS;

    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0; //无阻塞

    io_baud = to_baud(baud);
    if(tcflush(*fd,TCIOFLUSH) < 0 
    || cfsetispeed(&tio,io_baud) < 0 
    || cfsetospeed(&tio,io_baud) < 0 
    || tcsetattr(*fd,TCSANOW,&tio) < 0)
        return -1;
    return 0;


}
//函数静态定义
static int p_open(int* fd ,const char* port_name ,int baud,bool rtcstr)
{
    if(fd == NULL)
    {
        errno = EINVAL;
        return -1;
    }
    *fd = open(port_name,O_RDWR|O_NOCTTY|O_NONBLOCK);
    if(*fd == -1)
    {
        return -1;
    }
    if(p_set_termios(fd,baud,rtcstr) < 0)
    {
        int saved = errno;
        close(*fd);
        errno = saved;
        *fd = -1;
        return -1;
    }
    return 0;
    
}

static ssize_t nb_write_once(const int fd, const void* buff, size_t len)
{
    ssize_t write_len = 0;
    if(fd < 0 || buff == NULL)
    {
        errno = EINVAL;
        return -1;
    }
#if defined(SSIZE_MAX)
    if(len > SSIZE_MAX)
    {
        errno = EINVAL;
        return -1;
    }
#endif
    if(len == 0)
        return 0;//空操作

    while(true)
    {
        write_len = write(fd,buff,len);
        if(write_len >= 0)
            return write_len;
        if(errno == EINTR)
            continue;
        //EAGAIN/EWOULDBLOCK保持errno,上层调用判断"暂不可写"
        return -1;
    }
}

static ssize_t nb_read_once(const int fd, void* buff, size_t len)
{
    ssize_t read_len;
    if(fd < 0 || buff == NULL)
    {
        errno = EINVAL;
        return -1;
    }
#if defined(SSIZE_MAX)
    if(len > SSIZE_MAX)
    {
        errno = EINVAL;
        return -1;
    }
#endif
    if(len == 0)
        return 0;//空操作

    while(true)
    {
        read_len = read(fd,buff,len);
        if(read_len >= 0)
            return read_len; //0代表EOF
        if(errno == EINTR)
            continue;        //中断重试
        //EAGAIN/EWOULDBLOCK保持errno,上层调用判断"暂无内容"
        return -1;
    }
}

static int p_close(int* fd)
{
    if(fd == NULL || *fd < 0)
    {
        errno = EINVAL;
        return -1;
    }
    close(*fd);
    *fd = -1;
    return 0;
    
}

int app_serial_init(serial_port_t *app_serial, const char* io_path, int baud, bool rtcstr)
{
    app_serial->sp_open = p_open;
    app_serial->sp_write = nb_write_once;
    app_serial->sp_read = nb_read_once;
    app_serial->sp_close = p_close;
    return app_serial->sp_open(&(app_serial->fd),io_path,baud,rtcstr);
    
}

void errExit(const char* fmt,...)
{
    int err = errno;
    char msg[512];
    char errmess[128];
    va_list ap;
    va_start(ap,fmt);
    vsnprintf(msg,sizeof(msg),fmt,ap);
    strerror_r(err,errmess,sizeof(errmess));
#if LOG_ENABLE
    fprintf(stderr,"[ERROR]%s:%s",msg,errmess);
#else
#endif
    va_end(ap);
    exit(EXIT_FAILURE);
}

void app_sleep_ms(uint32_t ms)
{
    struct timespec s_time_t;
    s_time_t.tv_sec = ms / 1000;
    s_time_t.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&s_time_t,NULL);

}
