#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int fd, fd_l ,ret;
    char *filename, led_value = 0;
    unsigned int key_value;

    if(argc != 2)
    {
        printf("Error Usage\r\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename, O_RDWR);
    if(fd < 0)
    {
        printf("file %s open failed\r\n", argv[1]);
        return -1;
    }

    while(1)
    {
        ret = read(fd, &key_value, sizeof(key_value));
        if(ret < 0)
        {
            printf("read failed\r\n");
            break;
        }
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

    ret = close(fd);
    if(ret < 0)
    {
        printf("file %s close failed\r\n", argv[1]);
        return -1;
    }

    return 0;
}
























