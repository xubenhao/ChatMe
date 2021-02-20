#ifndef NLIB_TCPCONNECTION_H
#define NLIB_TCPCONNECTION_H

#include "../Lib/lib.h"
struct tcp_info;
class Channel;
class EventLoop;
class Socket;

class TcpConnection : 
    public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(
        EventLoop* loop,
        const string& name,
        int sockfd,
        const InetAddress& localAddr,
        const InetAddress& peerAddr);
    
    ~TcpConnection();

    // 通过公共接口，
    // 允许让可访问到此TcpConnection对象的任何线程
    // 与已经连接套接字交互
    // 包括：
    // １．获取已经连接套接字的语境信息
    // ２．向已经连接套接字写入信息
    // ３．控制已经连接套接字，如关闭，关闭写，．．．
    // ４．设置已经连接套接字的事件回调
    // 目前支持的事件回调有
    // ａ．收到完整应用层消息
    // ｂ．写入一条完整消息
    // ｃ．连接建立／连接关闭
    // ｄ．待写入数据超过一个指定界限
    EventLoop* getLoop() const
    {
        return m_pLoop;
    }

    const string& name() const
    {
        return m_strName;
    }
    
    const InetAddress& localAddress() const
    {
        return m_nLocalAddr;
    }

    const InetAddress& peerAddress() const
    {
        return m_nPeerAddr;
    }

    bool connected() const
    {
        return m_nState == s_nConnected;
    }

    bool disconnected() const
    {
        return m_nState == s_nDisconnected;
    }

    bool getTcpInfo(
            struct tcp_info*) const;
    string getTcpInfoString() const;
    
    void send(
            const void* message, 
            int len);
    
    void send(
            const StringPiece& message);
    
    void send(
            Buffer* message);
    
    void shutdown();
    void forceClose();
    void forceCloseWithDelay(
            double seconds);
    void setTcpNoDelay(bool on);
    
    void startRead();
    void stopRead();
    
    bool isReading() const
    {
        return m_bReading;
    };
    
    void setContext(
            const boost::any& context)
    {
        m_nContext = context;
    }

    const boost::any& getContext() const
    {
        return m_nContext;
    }
        
    boost::any* getMutableContext()
    {
        return &m_nContext;
    }
    
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
    
    void setHighWaterMarkCallback(
        const HighWaterMarkCallback& cb, 
        size_t highWaterMark)
    {
        m_nHighWaterMarkCallback = cb;
        m_nHighWaterMark = highWaterMark;
    }

    Buffer* inputBuffer()
    {
        return &m_nInputBuffer;
    }
    
    Buffer* outputBuffer()
    {
        return &m_nOutputBuffer;
    }

    void setCloseCallback(
            const CloseCallback& cb)
    {
        m_nCloseCallback = cb;
    }

    void setErrCallback(
            const ErrorCallback& cb)
    {
        m_nErrCallback = cb;
    }

    void connectEstablished();
    void connectDestroyed();
private:
    enum StateE 
    { 
        s_nDisconnected, 
        s_nConnecting, 
        s_nConnected, 
        s_nDisconnecting 
    };
    
    void handleRead(
            TimeStamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
    void sendInLoop(
            const StringPiece& message);
    void sendInLoop(
            const void* message, size_t len);
    void shutdownInLoop();
    void forceCloseInLoop();
    void setState(StateE s) { m_nState = s; }
    const char* stateToString() const;
    void startReadInLoop();
    void stopReadInLoop();
    
private:
    EventLoop* m_pLoop;
    const string m_strName;
    StateE m_nState;
    bool m_bReading;
    
    // 使用unique_ptr进行资源管理
    std::unique_ptr<Socket> m_pSocket;
    std::unique_ptr<Channel> m_pChannel;
    
    const InetAddress m_nLocalAddr;
    const InetAddress m_nPeerAddr;
    
    ConnectionCallback m_nConnectionCallback;
    MessageCallback m_nMessageCallback;
    WriteCompleteCallback m_nWriteCompleteCallback;
    HighWaterMarkCallback m_nHighWaterMarkCallback;
    CloseCallback m_nCloseCallback;
    ErrorCallback m_nErrCallback;
    
    size_t m_nHighWaterMark;
    Buffer m_nInputBuffer;
    Buffer m_nOutputBuffer;

    boost::any m_nContext;
};

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::weak_ptr<TcpConnection> TcpConnectionWeakPtr;
#endif
