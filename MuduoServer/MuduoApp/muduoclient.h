//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#ifndef MUDUO_APP_MUDUOCLIENT_H
#define MUDUO_APP_MUDUOCLIENT_H
#include "header.h"
#include "codec.h"

void LogOutput(const char* msg, int len);

class MuduoClient
{
public:
    static MuduoClient* instance(EventLoop* loop, const InetAddress& serverAddr);
    void connect();
    void disconnect();
    void write(const StringPiece& message);
private:
    void onConnection(const TcpConnectionPtr& conn);
    void onStringMessage(
          const TcpConnectionPtr&,
          const string& message,
          TimeStamp);

private:
    MuduoClient(EventLoop* loop, const InetAddress& serverAddr);

private:
    static MuduoClient* __instance;

private:
    TcpClient m_nClient;
    LengthHeaderCodec m_nCodec;
    MutexLock m_nMutex;
    TcpConnectionPtr m_nConnection;
};

#endif // MUDUOCLIENT_H
