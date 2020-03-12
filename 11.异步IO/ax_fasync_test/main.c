/** ===================================================== **
 *Author : ALINX Electronic Technology (Shanghai) Co., Ltd.
 *Website: http://www.alinx.com
 *Address: Room 202, building 18,
           No.518 xinbrick Road,
           Songjiang District, Shanghai
 *Created: 2020-3-2
 *Version: 1.0
 ** ===================================================== **/

#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "poll.h"
#include "sys/select.h"
#include "sys/time.h"
#include "linux/ioctl.h"
#include "signal.h"

static int fd = 0, fd_l = 0;

static void sigio_signal_func()
{
    int ret = 0;
    static char led_value = 0;
    unsigned int key_value;

    /* 获取按键状态 */
    ret = read(fd, &key_value, sizeof(key_value));
    if(ret < 0)
    {
        printf("read failed\r\n");
    }

    /* 判断按键状态 */
    if(1 == key_value)
    {
        /* 按键被按下，改变吗led状态 */
        printf("ps_key1 press\r\n");
        led_value = !led_value;

        fd_l = open("/dev/gpio_leds", O_RDWR);
        if(fd_l < 0)
        {
            printf("file /dev/gpio_leds open failed\r\n");
        }

        ret = write(fd_l, &led_value, sizeof(led_value));
        if(ret < 0)
        {
            printf("write failed\r\n");
        }

        ret = close(fd_l);
        if(ret < 0)
        {
            printf("file /dev/gpio_leds close failed\r\n");
        }
    }
}

int main(int argc, char *argv[])
{
    int flags = 0;
    char *filename;

    if(argc != 2)
    {
        printf("wrong para\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename, O_RDWR);
    if(fd < 0)
    {
        printf("can not open file %s\r\n", filename);
        return -1;
    }

    /* 指定信号SIGIO，并绑定处理函数 */
    signal(SIGIO, sigio_signal_func);
    /* 把当前线程指定为将接收信号的进程 */
    fcntl(fd, F_SETOWN, getpid());
    /* 获取当前线程状态 */
    flags = fcntl(fd, F_GETFD);
    /* 设置当前线程为FASYNC状态 */
    fcntl(fd, F_SETFL, flags | FASYNC);

    while(1)
    {
        sleep(2);
    }

    close(fd);
    return 0;
}






