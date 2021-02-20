#ifndef NLIB_ACCEPTOR_H
#define NLIB_ACCEPTOR_H

#include "../Lib/lib.h"  
class EventLoop;
class InetAddress;
class Acceptor 
{
public:
    typedef std::function<void (
        int sockfd, 
        const InetAddress&)> NewConnectionCallback;
    Acceptor(
        EventLoop* loop, 
        const InetAddress& listenAddr, 
        bool reuseport);
    ~Acceptor();
    void setNewConnectionCallback(const NewConnectionCallback& cb)
    {
        m_nNewConnectionCallback = cb;
    }
    
    bool listenning() const 
    { 
        return m_bListenning; 
    }
    
    void listen();
    
private:
    void handleRead();
    EventLoop* m_pLoop;
    Socket m_nAcceptSocket;
    Channel m_nAcceptChannel;
    NewConnectionCallback m_nNewConnectionCallback;
    bool m_bListenning;
    int m_nIdleFd;
};

#endif
