#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h> 
#define IPADDRESS   "127.0.0.1"
#define PORT        8787
#define MAXSIZE     1024
#define LISTENQ     5
#define FDSIZE      1000
#define EPOLLEVENTS 100 

// 借助epoll实现一对一echo

static int socket_bind(const char* ip,int port);
//IO多路复用epoll
static void do_epoll(int listenfd);
//事件处理函数
static void handle_events(
    int epollfd,
    struct epoll_event *events,
    int num,
    int listenfd,
    char *buf);
 //处理接收到的连接
 static void handle_accpet(
    int epollfd,
    int listenfd);
 //读处理
 static void do_read(
    int epollfd,
    int fd,
    char *buf);
 //写处理
 static void do_write(
    int epollfd,
    int fd,
    char *buf);
 //添加事件
 static void add_event(
    int epollfd,
    int fd,
    int state);
 //修改事件
 static void modify_event(
    int epollfd,
    int fd,
    int state);
 //删除事件
 static void delete_event(
    int epollfd,
    int fd,
    int state);
 
int main(int argc,char *argv[])
{
    int  listenfd;
    // 创建监听套接字
    listenfd = socket_bind(IPADDRESS,PORT);
    // 监听
    listen(listenfd,LISTENQ);
    // 执行I/O Multiplex
    do_epoll(listenfd);
    return 0;
}
 
static int socket_bind(
    const char* ip,
    int port)
 {
    int  listenfd;
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    if (listenfd == -1)
    {
        perror("socket error:");
        exit(1);
    }
 
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(
        AF_INET,
        ip,
        &servaddr.sin_addr);
    servaddr.sin_port = htons(port);
    if (bind(
        listenfd,
        (struct sockaddr*)&servaddr,
        sizeof(servaddr)) == -1)
    {
        perror("bind error: ");
        exit(1);
    }
    
    return listenfd;
}
 
static void do_epoll(int listenfd)
{
    int epollfd;
    struct epoll_event events[EPOLLEVENTS];
    int ret;
    char buf[MAXSIZE];
    memset(buf,0,MAXSIZE);
    //创建一个描述符
    epollfd = epoll_create(FDSIZE);
    //添加监听描述符事件--对监听套接字，监听其可读
    add_event(
        epollfd,
        listenfd,
        EPOLLIN);
    for ( ; ; )
    {
        // 阻塞等待
        // 可以阻塞等待描述符集合
        // 这就是I/O Multiplex
        ret = epoll_wait(
                epollfd,
                events,
                EPOLLEVENTS,
                -1);
        // 处理
        handle_events(
            epollfd,
            events,
            ret,
            listenfd,
            buf);
    }
     
    close(epollfd);
} 

static void handle_events(
    int epollfd,
    struct epoll_event *events,
    int num,
    int listenfd,
    char *buf)
{
    int i;
    int fd;
    //进行选好遍历
    for (i = 0;i < num;i++)
    {
        fd = events[i].data.fd;
        // 对每个注册的描述符允许绑定一个自定义数据
        //
        //
        // 这里借助自定义数据来辨别
        if ((fd == listenfd) 
                &&(events[i].events & EPOLLIN))
        {
            // 处理新监听套接字上的已经连接套接字
            handle_accpet(
                epollfd,
                listenfd);
        }
        else if (events[i].events & EPOLLIN)
        {
            // 对已经连接套接字可读的处理
            do_read(
                epollfd,
                fd,
                buf);
        }
        else if (events[i].events & EPOLLOUT)
        {
            // 对已经连接套接字可写的处理
            do_write(
                epollfd,
                fd,
                buf);
        }
    }
}

static void handle_accpet(
    int epollfd,
    int listenfd)
{
    int clifd;
    struct sockaddr_in cliaddr;
    socklen_t  cliaddrlen;
    // 先是
    // 对监听套接字执行accept
    // 得到已经连接套接字
    //
    // 与I/OMultiplex搭配的应该是非阻塞套接字
    // 才能避免虚假唤醒下，持续阻塞与一个操作
    // 而导致事件处理卡在一个结点
    // 使得后续事件，及事件监听被阻塞的情况
    clifd = accept(
        listenfd,
        (struct sockaddr*)&cliaddr,
        &cliaddrlen);
    if (clifd == -1)
    {
        perror("accpet error:");
    }
    else
    {
        printf(
            "accept a new client: %s:%d\n",
            inet_ntoa(cliaddr.sin_addr),
            cliaddr.sin_port);
        //添加一个客户描述符和事件
        add_event(
            epollfd,
            clifd,
            EPOLLIN);
    }
}
 
static void do_read(
    int epollfd,
    int fd,
    char *buf)
{
    int nread;
    nread = read(fd,buf,MAXSIZE);
    if (nread == -1)
    {
        perror("read error:");
        close(fd);
        delete_event(epollfd,fd,EPOLLIN);
    }
    else if (nread == 0)
    {
        fprintf(stderr,"client close.\n");
        close(fd);
        delete_event(
            epollfd,
            fd,
            EPOLLIN);
    }
    else
    {
        printf("read message is : %s",buf);
        //修改描述符对应的事件，由读改为写
        //
        modify_event(
            epollfd,
            fd,
            EPOLLOUT);
    }
}
 
static void do_write(
    int epollfd,
    int fd,
    char *buf)
{
    int nwrite;
    nwrite = write(fd,buf,strlen(buf));
    if (nwrite == -1)
    {
        perror("write error:");
        close(fd);
        delete_event(
            epollfd,
            fd,
            EPOLLOUT);
    }
    else
    {
        modify_event(
            epollfd,
            fd,
            EPOLLIN);
    }
   
    memset(buf,0,MAXSIZE);
}
 
static void add_event(
    int epollfd,
    int fd,
    int state)
{
    struct epoll_event ev;
    // 监听描述符的啥事件
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
        fd,
        &ev);
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
