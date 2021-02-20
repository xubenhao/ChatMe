#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
 
#define MAXSIZE     1024
#define IPADDRESS   "127.0.0.1"
#define SERV_PORT   8787
#define FDSIZE        1024
#define EPOLLEVENTS 20 
static void handle_connection(int sockfd);
static void handle_events(
    int epollfd,
    struct epoll_event *events,
    int num,
    int sockfd,
    char *buf);
static void do_read(
    int epollfd,
    int fd,
    int sockfd,
    char *buf);
static void do_read(
    int epollfd,
    int fd,
    int sockfd,
    char *buf);
static void do_write(
    int epollfd,
    int fd,
    int sockfd,
    char *buf);
static void add_event(
    int epollfd,
    int fd,
    int state);
static void delete_event(
    int epollfd,
    int fd,
    int state);
static void modify_event(
    int epollfd,
    int fd,
    int state);

int main(int argc,char *argv[])
{
    int sockfd;
    struct sockaddr_in  servaddr;
    sockfd = socket(
        AF_INET,
        SOCK_STREAM,
        0);
    bzero(
        &servaddr,
        sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    inet_pton(
        AF_INET,
        IPADDRESS,
        &servaddr.sin_addr);
    // 发起连接请求
    // 这里简单演示下，均为阻塞套接字
    connect(
        sockfd,
        (struct sockaddr*)&servaddr,
        sizeof(servaddr));
    //处理连接
    handle_connection(sockfd);
    close(sockfd);
    return 0;
} 
 
static void handle_connection(int sockfd)
{
    int epollfd;
    struct epoll_event events[EPOLLEVENTS];
    char buf[MAXSIZE];
    int ret;
    epollfd = epoll_create(FDSIZE);
    // 对终端输入进行监听－－可读
    add_event(
        epollfd,
        STDIN_FILENO,
        EPOLLIN);
    for ( ; ; )
    {
        ret = epoll_wait(
            epollfd,
            events,
            EPOLLEVENTS,
            -1);
        
        handle_events(
            epollfd,
            events,
            ret,
            sockfd,buf);
    }

    close(epollfd);
}
 
static void handle_events(
    int epollfd,
    struct epoll_event *events,
    int num,
    int sockfd,
    char *buf)
{
    int fd;
    int i;
    for (i = 0;i < num;i++)
    {
        fd = events[i].data.fd;
        // 执行读终端
        if (events[i].events & EPOLLIN)
        {
            do_read(
                epollfd,
                fd,
                sockfd,
                buf);
        }
        else if (events[i].events & EPOLLOUT)
        {
            // 执行向套接字写入
            // 然后等待其可读
            do_write(
                epollfd,
                fd,
                sockfd,
                buf);
        }
    }
}
 
static void do_read(
    int epollfd,
    int fd,
    int sockfd,
    char *buf)
{
    int nread;
    nread = read(fd,buf,MAXSIZE);
    if (nread == -1)
    {
        perror("read error:");
        close(fd);
    }
    else if (nread == 0)
    {
        fprintf(stderr,"server close.\n");
        close(fd);
    }
    else
    {
        if (fd == STDIN_FILENO)
        {
            // 有了内容要写入套接字
            // 这里先注册其可写事件
            // 在事件通知处理时写，而非立即写
            add_event(
                epollfd,
                sockfd,
                EPOLLOUT);
        }
        else
        {
            // 从套接字读取事件中读出内容
            // 取消套接字注册
            // 内容写入终端－－不立即写入
            // 而是先注册终端写事件
            // 在事件处理中写
            // 
            delete_event(
                epollfd,
                sockfd,
                EPOLLIN);
            add_event(
                epollfd,
                STDOUT_FILENO,
                EPOLLOUT);    
        }    
    }
}
 
static void do_write(
    int epollfd,
    int fd,
    int sockfd,
    char *buf)
{
    int nwrite;
    nwrite = write(
        fd,
        buf,
        strlen(buf));
    if (nwrite == -1)
    {
        perror("write error:");
        close(fd);
    }
    else
    {
        // 对终端写的处理
        // 将内容写到终端
        // 删除对终端可写监听
        //
        // 终端读／终端写用了独立的描述符
        if (fd == STDOUT_FILENO)
        {
            delete_event(
                    epollfd,
                    fd,
                    EPOLLOUT);
        }
        else
        {
            // 对套接字写的处理
            // 内容写入套接字
            // 转而监听套接字可读
            modify_event(
                epollfd,
                fd,
                EPOLLIN);
        }
    }

    memset(buf,0,MAXSIZE);
}
 
static void add_event(
    int epollfd,
    int fd,
    int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(
        epollfd,
        EPOLL_CTL_ADD,
        fd,
        &ev);
}
 
static void delete_event(
    int epollfd,
    int fd,
    int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(
        epollfd,
        EPOLL_CTL_DEL,
        fd,&ev);
}
 
static void modify_event(
    int epollfd,
    int fd,
    int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(
        epollfd,
        EPOLL_CTL_MOD,
        fd,
        &ev);
}
