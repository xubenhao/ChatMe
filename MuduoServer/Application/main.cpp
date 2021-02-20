//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//
#include "main.h"

int main(int argc, char *argv[])
{
    LOG_INFO
        << "pid = "
        << getpid();
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi("20201"));
    InetAddress serverAddr(port);
    char _strHost[] = "127.0.0.1";
    char _strUser[] = "muduoclient";
    char _strPassword[] = "370970843";
    MuduoServer server(
        &loop,
        serverAddr,
        _strHost,
        strlen(_strHost),
        _strUser,
        strlen(_strUser),
        _strPassword,
        strlen(_strPassword));

    server.setThreadNum(atoi("3"));
    server.start();
    loop.loop();
    int i = 0;
    i++;
}
