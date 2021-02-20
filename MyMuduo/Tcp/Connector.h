#ifndef NLIB_CONNECTOR_H
#define NLIB_CONNECTOR_H
#include "../Lib/lib.h"

class Channel;
class EventLoop;

class Connector : 
    public std::enable_shared_from_this<Connector>
{
public:
    typedef std::function<void (int sockfd)> NewConnectionCallback;
    Connector(
        EventLoop* loop, 
        const InetAddress& serverAddr);
    ~Connector();
    
    void setNewConnectionCallback(
        const NewConnectionCallback& cb)
    {
        m_nNewConnectionCallback = cb;
    }

    void start();
    void restart();
    void stop();
    
    const InetAddress& serverAddress() const
    {
        return m_nServerAddr;
    }
    
private:
    enum States
    {
        s_nDisconnected,
        s_nConnecting,
        s_nConnected
    };

    static const int s_nMaxRetryDelayMs = 30*1000;
    static const int s_nInitRetryDelayMs = 500;
    void setState(States s)
    {
        m_nState = s;
    }
    
    void startInLoop();
    void stopInLoop();
    void connect();
    void connecting(int sockfd);
    void handleWrite();
    void handleError();
    void retry(int sockfd);
    int removeAndResetChannel();
    void resetChannel();
    
private:
    EventLoop* m_pLoop;
    InetAddress m_nServerAddr;
    
    bool m_bConnect;
    States m_nState;
    std::unique_ptr<Channel> m_pChannel;
    NewConnectionCallback m_nNewConnectionCallback;
    int m_nRetryDelayMs;
};


#endif
