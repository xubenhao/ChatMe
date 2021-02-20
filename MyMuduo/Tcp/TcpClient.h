#ifndef NLIB_TCPCLIENT_H
#define NLIB_TCPCLIENT_H

#include "../Lib/lib.h"
#include "TcpConnection.h"

class Connector;
// Muduo的事件循环／线程池／客户端／服务端／Io复用总结：
// １．一个EventLoop的loop独占一个线程
// EventLoop本身构成了此线程执行的语境
// ２．利用EventLoop内的epoll描述符，
// 事件循环可以对一个或多个描述符进行监听
// 监听的事件可以指定
// 事件循环还支持任务回调，其他可访问到EventLoop的线程，
// 可向其任务回调写入回调来提交任务．
// ３．每个描述符提供给epoll监听时允许绑定一个自定义数据
// Muduo里每个描述符绑定一个指向Channel对象的指针，
// 此Channel对象构成了每个描述符的语境．
// 通过Channel语境可以提供描述符
// 当前与epoll的关系／要监听的事件／产生的事件／事件回调／．．．
// ４．一个TcpClient代表一个客户端
// 提供的接口connect/disconnect/stop允许触发
// 套接字创建／连接请求发起／连接关闭／．．．
// ５．Muduo的事件循环既监听多个描述符，
// 在事件发生时触发事件回调
// 又支持任务回调．
// 这样要求事件循环监听的每个描述符／处理的每个任务回调
// 必须在一个短的时间内完成事件处理／任务回调，
// 这样才能保证高响应性．
// 防止阻塞与一个处理，导致后续处理，延迟较大才响应情况．
// 为此，监听的描述符需要是非阻塞描述符
// 处理的任务回调需要是可以是快速处理完毕的任务回调
// ６．非阻塞客户端流程：
// ａ．创建套接字＋发起连接请求
// ｂ．创建套接字语境Channel＋事件循环监听套接字可读
// ｃ．处理可读事件，处理结果是产生一个TcpConnecton对象代表一个已经连接套接字的语境

// ｄ．已经连接套接字，需要有其Channel语境＋事件循环监听套接字可读
// 对套接字可读的监听，在连接建立期间一直如此
// 仅在有要写到套接字的内容存储于缓冲区时，才需要监听其可写．
// 且在可写事件中，写入缓冲区数据后，若无数据等待写入，
// 则关闭对其可写监视
//
// ｅ．连接关闭
// 主动发起时，
// 待收到对端的FIN，会触发读事件
// 读事件处理中，读０字节表示收到对端FIN，可关闭连接
// 执行清理－－删除在epoll中对此描述符监视／回收套接字的语境Channel
//
// ７．非阻塞服务端流程：
// ａ．创建套接字＋开始监听
// ｂ．创建监听套接字语境Channel＋事件循环监听套接字可读
// ｃ．处理可读事件，
// 处理结果是产生一个TcpConnection对象代表一个已经连接套接字语境
// ｄ．已经连接套接字，需要有其Channel语境＋事件循环监听套接字可读
// 对套接字可读的监听，在连接建立期间一直如此
// 仅在有要写到套接字的内容存储于缓冲区时，才需要监听其可写．
// 且在可写事件中，写入缓冲区数据后，若无数据等待写入，
// 则关闭对其可写监视
//
// ｅ．连接关闭
// 主动发起时，
// 待收到对端的FIN，会触发读事件
// 读事件处理中，读０字节表示收到对端FIN，可关闭连接
// 执行清理－－删除在epoll中对此描述符监视／回收套接字的语境Channel
// 服务器对象可以保存所有存在的已经连接套接字及其他信息
// 实现管理
// ８．服务端的多线程模型
// 监听套接字依附的事件循环所在的线程，负责监听
// 服务器通过接口启动多个并行线程－－即线程池
// 监听套接字可读处理时，
// 产生的每个已经连接套接字按负载均衡方式分配非线程池中线程
typedef std::shared_ptr<Connector> ConnectorPtr;
class TcpClient 
{
public:
    TcpClient(
        EventLoop* loop,// 一个独立线程的事件循环
        const InetAddress& serverAddr,// 目的端地址
        const string& nameArg);
    ~TcpClient();  
    
    void connect();
    void disconnect();
    void stop();
    
    TcpConnectionPtr connection() const
    {
        MutexLockGuard lock(m_nMutex);
        return m_nConnection;
    }

    EventLoop* getLoop() const
    {
        return m_pLoop;
    }
    
    bool retry() const
    {
        return m_bRetry;
    }

    void enableRetry()
    {
        m_bRetry = true;
    }

    const string& name() const
    {
        return m_strName;
    }
    
    void setConnectionCallback(
            ConnectionCallback cb)
    {
        m_nConnectionCallback = std::move(cb);
    }
    
    void setMessageCallback(
            MessageCallback cb)
    {
        m_nMessageCallback = std::move(cb);
    }
    
    void setWriteCompleteCallback(
            WriteCompleteCallback cb)
    {
        m_nWriteCompleteCallback = std::move(cb);
    }

private:
    void newConnection(int sockfd);
    void removeConnection(const TcpConnectionPtr& conn);

private:
    EventLoop* m_pLoop;
    ConnectorPtr m_nConnector; 
    const string m_strName;

    ConnectionCallback m_nConnectionCallback;
    MessageCallback m_nMessageCallback;
    WriteCompleteCallback m_nWriteCompleteCallback;
    
    bool m_bRetry;   
    bool m_bConnect; 
    int m_nNextConnId;
    
    mutable MutexLock m_nMutex;
    TcpConnectionPtr m_nConnection;
};
    
#endif
