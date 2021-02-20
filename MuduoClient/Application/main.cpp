//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#include "main.h"

int main(int argc, char *argv[])
{
    Logger::setOutput(LogOutput);
    LOG_TRACE
        << "pid = "
        << getpid();

    EventLoopThread loopThread;
    uint16_t port = (uint16_t)(atoi("20201"));
    if(argc != 2)
    {
        InetAddress serverAddr("127.0.0.1", port);
        // 客户端连接发起＋已经连接套接字处理放在一个线程
        MuduoClient* _pClient = MuduoClient::instance(loopThread.startLoop(), serverAddr);
        _pClient->connect();
    }
    else
    {
        InetAddress serverAddr(argv[1], port);
        // 客户端连接发起＋已经连接套接字处理放在一个线程
        MuduoClient* _pClient = MuduoClient::instance(loopThread.startLoop(), serverAddr);
        _pClient->connect();
    }

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
