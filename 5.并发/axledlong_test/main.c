#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
    int fd;
    char buf;
    int count;

    if(3 != argc)
    {
        printf("none para\n");
        return -1;
    }

    fd = open(argv[1], O_RDWR);
    if(fd < 0)
    {
        printf("Can't open file %s\r\n", argv[1]);
        return -1;
    }

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
        printf("wrong para\n");
        return -2;
    }

    count = 20;
    while(count --)
    {
        sleep(1);
    }

    close(fd);
    return 0;
}
