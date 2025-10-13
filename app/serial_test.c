#include"communication_port.h"

long str_to_int(char* strNumber)
{
    long result = 0;
    if(strNumber == NULL)
        return -1;
    result = strtol(strNumber,NULL,10);
    if((result == LONG_MAX && errno == ERANGE) || (result == LONG_MIN && errno == ERANGE))
        return -2;
    else 
        return result;
}

int str_to_bool(char* strbool,bool* type)
{
    long result = 0;
    if(strbool == NULL || type == NULL)
        return -1;
    result = strtol(strbool,NULL,10);
    if(result < 0 || result > 1)
    {
        return -1;
    }
    else
    {
        *type = result ? 1:0;
        return 0;
    }
}

int main(int argc,char **argv)
{
    // serial_port_t sp;
    long baud = 115200;
    bool rctstr_type = 0;
    if(argc < 4)
    {
        printf("Bad apply :please apply witch: appName pathName baudRate rctstrBool\r\n");
        exit(EXIT_FAILURE);
    }
    baud = str_to_int(argv[2]);
    if(baud < 0)
    {
        errExit("str_to_int");
    }
    if(str_to_bool(argv[3],&rctstr_type) < 0)
    {
        printf("memeber 3 is not bool type\r\n");
        exit(EXIT_FAILURE);
    }

    if(app_serial_init(&sp,argv[1],(int)baud,rctstr_type) < 0)
        errExit("app_serial_init");
    
    

    
}