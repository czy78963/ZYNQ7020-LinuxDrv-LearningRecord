/** ===================================================== **
 *Author : ALINX Electronic Technology (Shanghai) Co., Ltd.
 *Website: http://www.alinx.com
 *Address: Room 202, building 18, 
           No.518 xinbrick Road, 
           Songjiang District, Shanghai
 *Created: 2020-3-2 
 *Version: 1.0
 ** ===================================================== **/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
    int fd;
    char buf;

    /* 验证输入参数个数 */
    if(3 != argc)
    {
        printf("none para\n");
        return -1;
    }

    /* 打开输入的设备文件, 获取文件句柄 */
    fd = open(argv[1], O_RDWR);
    if(fd < 0)
    {
        /* 打开文件失败 */
        printf("Can't open file %s\r\n", argv[1]);
        return -1;
    }

    /* 判断输入参数, on就点亮led, off则熄灭led */
    if(!strcmp("on",argv[2]))
    {
        printf("ps_led1 on\n");
        buf = 1;
        write(fd, &buf, 1);
    }
    else if(!strcmp("off",argv[2]))
    {
        printf("ps_led1 off\n");
        buf = 0;
        write(fd, &buf, 1);
    }
    else
    {
        /* 输入参数错误 */
        printf("wrong para\n");
        return -2;
    }

    /* 操作结束后关闭文件 */
    close(fd);
    return 0;
}
