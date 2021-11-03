#include "fcntl.h"
#include "termios.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int open_netserver(const char*ip,uint16_t port);
int open_serial(char *device);
void net2ser_Function(void);
void ser2net_Function(void);

pthread_t ser2net_pthread;
pthread_t net2ser_pthread;
int ser_id;
int net_id;

/*ip port ser-port ser-baud*/

int main(int argc, char *argv[])
{
    int ret;
    if (argc < 5)
    {
        return 0;
    }
    ser_id= open_serial(argv[3]);
    if(ser_id<=0)
    {
        close(ser_id);
        return 0;
    }
    net_id=open_netserver(argv[1],atoi(argv[2]));
    if(net_id<=0)
    {
        close(net_id);
        return 0;
    }
    ret = pthread_create(&ser2net_pthread, NULL, (void *)ser2net_Function, NULL);
    if (ret != 0)
    {
        printf(" ser2net pthread create failed\n");
        return 0;
    }
    ret = pthread_create(&net2ser_pthread, NULL, (void *)net2ser_Function, NULL);
    if (ret != 0)
    {
        printf(" ser2net pthread create failed\n");
        return 0;
    }
    while(1)
    {
        sleep(1);
    }
}
int open_serial(char *device)
{
    struct termios opt;
    int uart_fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd < 0)
    {
        printf("Open uart %s failed.\n", device);
        return -1;
    }
    tcgetattr(uart_fd, &opt);
    opt.c_cflag |= CLOCAL | CREAD;
    opt.c_cflag &= ~CSIZE;
    opt.c_cflag |= CS8;
    opt.c_cflag &= ~PARODD;
    opt.c_cflag &= ~CSTOPB;
    cfsetispeed(&opt, B9600);
    cfsetospeed(&opt, B9600);
    opt.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    //opt.c_oflag &= ~();
    opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    //opt.c_oflag &= ~(OPOST);
    tcflush(uart_fd, TCIFLUSH);
    if (tcsetattr(uart_fd, TCSANOW, &opt) < 0)
    {
        printf("Uart operation failed.\n");
        return -1;
    }
    return uart_fd;
}
int open_netserver(const char*ip,uint16_t port)
{
    int sockfd;
    struct sockaddr_in client;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        printf("TCP_DEBUG-Creating socket failed.\n");
        return -1;
    }
    bzero(&client, sizeof(client));
    client.sin_family = AF_INET;
    client.sin_port = htons(port);
    client.sin_addr.s_addr = inet_addr(ip);
    if (connect(sockfd, (const struct sockaddr *)&client, sizeof(struct sockaddr_in)) < 0)
    {
        perror("tcp error:");
        printf("TCP_DEBUG-connect error.\n");
        return -1;
    }
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    return sockfd;
}

void ser2net_Function(void)
{
    unsigned long start_time;
    unsigned long end_time;
    unsigned char message[1024];
    struct timeval tv;
    unsigned int read_index = 0;
    gettimeofday(&tv, NULL);
    start_time = tv.tv_sec * 1000 + tv.tv_usec / 1000; //millisecond
    while(1)
    {
        int len = read(ser_id, message + read_index, 255);
        if (len > 0)
        {
            read_index += len;
            gettimeofday(&tv, NULL);
            start_time = tv.tv_sec * 1000 + tv.tv_usec / 1000; //millisecond
            if (read_index >= 1024)
            {
                send(net_id,message,read_index,0);
                printf("ser rev:");
                for(int i=0;i<read_index;i++)
                {
                    printf("%.2x",message[i]);
                }
                printf("\n");
                read_index=0;
            }
        }
        else
        {
            gettimeofday(&tv, NULL);
            end_time = tv.tv_sec * 1000 + tv.tv_usec / 1000; //millisecond
            if (end_time - start_time >= 50 && read_index>0)
            {
                send(net_id,message,read_index,0);
                printf("ser rev:");
                for(int i=0;i<read_index;i++)
                {
                    printf("%.2x",message[i]);
                }
                printf("\n");
                read_index=0;
            }
            usleep(10);
        }
    }
}
void net2ser_Function(void)
{
    unsigned long start_time;
    unsigned long end_time;
    unsigned char message[1024];
    struct timeval tv;
    unsigned int read_index = 0;
    gettimeofday(&tv, NULL);
    start_time = tv.tv_sec * 1000 + tv.tv_usec / 1000; //millisecond
    while(1)
    {
        int len = recv(net_id, message + read_index, 255,0);
        if (len > 0)
        {
            read_index += len;
            gettimeofday(&tv, NULL);
            start_time = tv.tv_sec * 1000 + tv.tv_usec / 1000; //millisecond
            if (read_index >= 1024)
            {
                write(ser_id,message,read_index);
                printf("net rev:");
                for(int i=0;i<read_index;i++)
                {
                    printf("%.2x",message[i]);
                }
                printf("\n");
                read_index=0;
            }
        }
        else
        {
            gettimeofday(&tv, NULL);
            end_time = tv.tv_sec * 1000 + tv.tv_usec / 1000; //millisecond
            if (end_time - start_time >= 50 && read_index>0)
            {
                write(ser_id,message,read_index);
                printf("net rev:");
                for(int i=0;i<read_index;i++)
                {
                    printf("%.2x",message[i]);
                }
                printf("\n");
                read_index=0;
            }
            usleep(10);
        }
    }
}