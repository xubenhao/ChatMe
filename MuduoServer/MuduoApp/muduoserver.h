//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#ifndef MUDUO_APP_MUDUOSERVER_H
#define MUDUO_APP_MUDUOSERVER_H

#include "header.h"
#include "codec.h"

// TcpServer的用户通过TcpServer提供的接口来使用TcpServer的功能
// 内部的高效运作，正确性管理属于网络库的责任
//
// 提供的接口包含：
// １．开启服务器的监听
// ２．设置收到已经连接套接字信息时的回调
// ３．设置服务器的属性
// ４．关闭服务器
// 与客户端一样
// 因为一般TCP的
// 消息接收需要解码再交给应用
// 消息发送需要编码再交给TcpConnection
// 所以可TcpClient一样，TcpServer没有提供给TcpConnection发消息接口
// 应用层需要自己实现
// 原始消息－－编码－－发给TcpConnection

class Message
{
public:
    Message()
    {
        m_bDirection = true;
    }

    ~Message()
    {

    }

public:
    QByteArray m_nContent;
    bool m_bDirection;
    TimeStamp m_nTimeStamp;
};

class MuduoServer
{
public:
    // for client to server direction
    enum MessageType
    {
        REGISTER = 0,
        LOGIN = 1,
        ADD = 2,
        UPDATE_FRIEND_LIST = 3,
        OFF_LINE = 4,
        NEED_TO_PROCESS_USER_LIST = 5,
        CHAT = 6,
        GET_NEED_TO_PROCESS_MESSAGES = 7,
    };

    // for server to client direction
    enum ResMessageType
    {
        REGISTER_RES = 0,
        LOGIN_RES = 1,
        ADD_RES = 2,
        UPDATE_FRIEND_LIST_RES = 3,
        NEED_TO_PROCESS_USER_LIST_RES = 4,
        CHAT_RES = 5,
        NEW_MESSAGE_NOTIFICATION = 6,
        GET_NEED_TO_PROCESS_MESSAGES_RES = 7,
    };

public:
    MuduoServer(
        EventLoop* loop,
        const InetAddress& listenAddr,
        char* pStrHost_,
        int nHostLen_,
        char* pStrUser_,
        int nUserLen_,
        char* pStrPassword_,
        int nPasswordLen_);
    // tcp server with multiple threads consisting of thread pool
    void setThreadNum(int numThreads);
    // create socket and start listen, at the same starting watching the event of readable
    void start();
private:
    void threadInit(EventLoop* loop);
    // 不论连接回调由服务器端那个线程触发
    // 执行的回调函数都是这个
    // 且由于成员函数性质，所有线程执行回调时，
    // 均可获得一个多个线程共享的回调语境－－－一个共享的MuduoServer对象
    // the time of one client connection build and destroy
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(
        const TcpConnectionPtr& conn,
        Buffer* buf,
        TimeStamp receiveTime);

    // a place to decode the client's message
    // and dispatch the decoded message to process it
    void onStringMessage(
        const TcpConnectionPtr&,
        void* strMessage_,
        int nLength_,
        TimeStamp);

    bool ResForRegister(
        const TcpConnectionPtr& pTcpConnection_,
        int nRetCode_);
    bool PackRegisterResMessage(
        int nRetCode_,
        Buffer& nBuf_);


    bool ResForLogin(
        const TcpConnectionPtr& pTcpConnection_,
        int nRetCode_,
        TimeStamp nTimeStamp_ = TimeStamp());
    bool PackLoginResMessage(
        int nRetCode_,
        TimeStamp nTimeStamp_,
        Buffer& nBuf_);

    bool ResForAdd(
        const TcpConnectionPtr& pTcpConnection_,
        int nRetCode_);
    bool PackAddResMessage(
        int nRetCode_,
        Buffer& nBuf_);


    bool ResForUpdateFriendList(
        const TcpConnectionPtr& pTcpConnection_,
        const NDataStruct::DynArray<QByteArray>& arrUserIds_);
    bool PackUpdateFriendListResMessage(
        const NDataStruct::DynArray<QByteArray>& arrUserIds_,
        Buffer& nBuf_);


    bool ResForNeedToProcessUserList(
        const TcpConnectionPtr& pTcpConnection_,
        const NDataStruct::DynArray<QByteArray>& arrUserIds_);
    bool PackNeedToProcessUserListResMessage(
        const NDataStruct::DynArray<QByteArray>& arrUserIds_,
        Buffer& nBuf_);


    bool ResForGetNeedToProcessMessagesMessage(
        const TcpConnectionPtr& pTcpConnection_,
        const NDataStruct::DynArray<Message>& arrMessages_);
    bool PackGetNeedToProcessMessagesMessage(
        const NDataStruct::DynArray<Message>& arrMessages_,
        Buffer& nBuf_);

    bool ResForChatMessage(
        const TcpConnectionPtr& pTcpConnection_);
    bool PackChatResMessage(
        Buffer& nBuf_);

    bool ResForNewMessageNotification(
        const TcpConnectionPtr& pTcpConnection_,
        const QByteArray& nByteSender_);
    bool PackNewResMessageNotification(
        const QByteArray& nByteSender_,
        Buffer& nBuf_);

    // a place to pack message and sending to client
    // void PackAndSend(
    //      int nType_,
    //      );
    //void
    typedef std::set<TcpConnectionPtr> ConnectionList;
    typedef std::map<QByteArray, std::set<TcpConnectionWeakPtr>> ConnectionMapping;

    typedef NDataStruct::DynArray<std::pair<TimeStamp, TcpConnectionWeakPtr>> ComplexConnections;
    typedef std::shared_ptr<ComplexConnections> ComplexConnectionsPtr;

    typedef std::map<QByteArray, ComplexConnectionsPtr> ComplexConnectionsMapping;

    void distributeMessage(
          const string& message);

private:
    bool ProcessRegister(
        const TcpConnectionPtr& pTcpConnection_,
        char* pUserId_,
        int nUserIdLen_,
        char* pPassword_,
        int nPasswordLen_);

    bool ProcessLogin(
        const TcpConnectionPtr& pTcpConnection_,
        char* pUserId_,
        int nUserIdLen_,
        char* pPassword_,
        int nPasswordLen_);

    bool ProcessAdd(
        const TcpConnectionPtr& pTcpConnection_,
        char* pMasterUserId_,
        int nMasterUserIdLen_,
        char* pUserId_,
        int nUserIdLen_);

    bool ProcessUpdateFriendList(
        const TcpConnectionPtr& pTcpConnection_,
        char* pUserId_,
        int nUserIdLen_);

    bool ProcessNeedToProcessUserList(
        const TcpConnectionPtr& pTcpConnection_,
        char* pUserId_,
        int nUserIdLen_);

    bool ProcessChatMessage(
        const TcpConnectionPtr& pTcpConnection_,
        char* pSenderUserId_,
        int nSenderUserIdLen_,
        char* pReceiverUserId_,
        int nReceiverUserIdLen_,
        char* pChatContent_,
        int nChatContentLen_);

    bool ProcessGetNeedToProcessMessagesMessage(
        const TcpConnectionPtr& pTcpConnection_,
        char* pSenderUserId_,
        int nSenderUserIdLen_,
        char* pReceiverUserId_,
        int nReceiverUserIdLen_);

    bool ProcessOffLine(
        const TcpConnectionPtr& pTcpConnection_,
        char* pUserId_,
        int nUserIdLen_,
        const TimeStamp& nTimeStamp_);

private:
    TcpServer m_nServer;
    MySqlAgent m_nSqlAgent;
    //LengthHeaderCodec m_nCodec;

    // every thread in the thread pool has a ConnectionList object
    typedef ThreadLocalSingleton<ConnectionList> LocalConnections;

    MutexLock m_nMutex;
    // every thread in the thread pool has a EventLoop object
    std::set<EventLoop*> m_nLoops;


    MutexLock m_nMutexForMapping;
    ComplexConnectionsMapping m_mapComplexConnectionsMapping;
};

#endif // MUDUOSERVER_H
