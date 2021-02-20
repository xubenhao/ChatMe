#ifndef NLIB_TCPSERVER_H
#define NLIB_TCPSERVER_H
#include "../Lib/lib.h"
#include "TcpConnection.h"
#include "Acceptor.h"

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

class TcpServer 
{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;
    enum Option
    {
        s_nNoReusePort,
        s_nReusePort,
    };

    TcpServer(
        EventLoop* loop,
        const InetAddress& listenAddr,
        const string& nameArg,
        Option option = s_nNoReusePort);
    ~TcpServer();  
    const string& ipPort() const
    {
        return m_strIpPort;
    }
    
    const string& name() const
    {
        return m_strName;
    }

    EventLoop* getLoop() const
    {
        return m_pLoop;
    }

    void setThreadNum(int numThreads);
    void setThreadInitCallback(
            const ThreadInitCallback& cb)
    {
        m_nThreadInitCallback = cb;
    }

    std::shared_ptr<EventLoopThreadPool> threadPool()
    {
        return m_pThreadPool;
    }

    void start();
    void setConnectionCallback(
            const ConnectionCallback& cb)
    {
        m_nConnectionCallback = cb;
    }

    void setMessageCallback(
            const MessageCallback& cb)
    {
        m_nMessageCallback = cb;
    }

    void setWriteCompleteCallback(
            const WriteCompleteCallback& cb)
    {
        m_nWriteCompleteCallback = cb;
    }

private:
    void newConnection(
        int sockfd, 
        const InetAddress& peerAddr);
    void removeConnection(
        const TcpConnectionPtr& conn);
    void removeConnectionInLoop(
        const TcpConnectionPtr& conn);
    
private:
    typedef std::map<string, TcpConnectionPtr> ConnectionMap;
    EventLoop* m_pLoop;
    const string m_strIpPort;
    const string m_strName;
    std::unique_ptr<Acceptor> m_pAcceptor;
    std::shared_ptr<EventLoopThreadPool> m_pThreadPool;
    
    ConnectionCallback m_nConnectionCallback;
    MessageCallback m_nMessageCallback;
    WriteCompleteCallback m_nWriteCompleteCallback;
    ThreadInitCallback m_nThreadInitCallback;
    
    AtomicInt32 m_nStarted;
    int m_nNextConnId;
    ConnectionMap m_nConnections;
};
   
#endif
