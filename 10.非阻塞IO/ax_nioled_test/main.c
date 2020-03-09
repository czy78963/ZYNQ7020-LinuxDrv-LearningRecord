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

int main(int argc, char *argv[])
{

    /* ret获取返回值, fd获取文件句柄 */
    int ret, fd, fd_l;
    /* 定义一个监视文件读变化的描述符合集 */
    fd_set readfds;
    /* 定义一个超时时间结构体 */
    struct timeval timeout;

    char *filename, led_value = 0;
    unsigned int key_value;

    if(argc != 2)
    {
        printf("Error Usage\r\n");
        return -1;
    }

    filename = argv[1];
    /* 获取文件句柄, O_NONBLOCK表示非阻塞访问 */
    fd = open(filename, O_RDWR | O_NONBLOCK);
    if(fd < 0)
    {
        printf("can not open file %s\r\n", filename);
        return -1;
    }

    while(1)
    {
        /* 初始化描述符合集 */
        FD_ZERO(&readfds);
        /* 把文件句柄fd指向的文件添加到描述符 */
        FD_SET(fd, &readfds);

        /* 超时时间初始化为1.5秒 */
        timeout.tv_sec = 1;
        timeout.tv_usec = 500000;

        /* 调用select, 注意第一个参数为fd+1 */
        ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
        switch (ret)
        {
            case 0:
            {
                /* 超时 */
                break;
            }
            case -1:
            {
                /* 出错 */
                break;
            }
            default:
            {
                /* 监视的文件可操作 */
                /* 判断可操作的文件是不是文件句柄fd指向的文件 */
                if(FD_ISSET(fd, &readfds))
                {
                    /* 操作文件 */
                    ret = read(fd, &key_value, sizeof(key_value));
                    if(ret < 0)
                    {
                        printf("read failed\r\n");
                        break;
                    }
                    printf("key_value = %d\r\n", key_value);
                    if(1 == key_value)
                    {
                        printf("ps_key1 press\r\n");
                        led_value = !led_value;

                        fd_l = open("/dev/gpio_leds", O_RDWR);
                        if(fd_l < 0)
                        {
                            printf("file /dev/gpio_leds open failed\r\n");
                            break;
                        }

                        ret = write(fd_l, &led_value, sizeof(led_value));
                        if(ret < 0)
                        {
                            printf("write failed\r\n");
                            break;
                        }

                        ret = close(fd_l);
                        if(ret < 0)
                        {
                            printf("file /dev/gpio_leds close failed\r\n");
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
    close(fd);
    return ret;
}
