#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "linux/ioctl.h"

int main(int argc, char *argv[])
{
    int fd, ret;
    char *filename;
    unsigned int interval_new, interval_old = 0;

    if(argc != 2)
    {
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];

    fd = open(filename, O_RDWR);
    if(fd < 0)
    {
        printf("can not open file %s\r\n", filename);
        return -1;
    }

    while(1)
    {
        printf("Input interval:");
        scanf("%d", &interval_new);

        if(interval_new != interval_old)
        {
            interval_old = interval_new;
            ret = write(fd, &interval_new, sizeof(interval_new));
            if(ret < 0)
            {
                printf("write failed\r\n");
            }
            else
            {
                printf("interval refreshed!\r\n");
            }
        }
        else
        {
            printf("same interval!");
        }
    }
    close(fd);
}


















