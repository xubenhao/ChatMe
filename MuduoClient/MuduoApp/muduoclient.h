//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#ifndef MUDUO_APP_MUDUOCLIENT_H
#define MUDUO_APP_MUDUOCLIENT_H
#include "header.h"
#include "codec.h"

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


void LogOutput(const char* msg, int len);

class MuduoClient
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
    // For Error
    void setErrorCallback(std::function<void(const TcpConnectionPtr&)> nFun_)
    {
        if(m_nConnection)
        {
            m_nConnection.get()->setErrCallback(nFun_);
        }
    }

private:
    std::function<void()> m_nErrorCallback;

public:
    // For Register
    bool sendRegisterMessage(const QString& strUserId_, const QString& strPassword_);
    void setRegisterCallback(std::function<void(int nType_)> nFun_)
    {
        m_nRegisterCallback = nFun_;
    }

private:
    bool PackRegisterMessage(const QString& strUserId_, const QString& strPassword_, Buffer& nBuf_);

private:
    std::function<void(int nType_)> m_nRegisterCallback;


public:
    // For get-need-to-process-messages
    bool sendGetNeedToProcessMessagesMessage(const QString& strSourceUserId_, const QString& strDestUserId_);
    void setGetNeedToProcessMessagesCallback(std::function<void(const NDataStruct::DynArray<Message>&)> nFun_)
    {
        m_nGetNeedToProcessMessagesCallback = nFun_;
    }

private:
    bool PackGetNeedToProcessMessagesMessage(const QString& strUserId_, const QString& strPassword_, Buffer& nBuf_);

private:
    std::function<void(const NDataStruct::DynArray<Message>&)> m_nGetNeedToProcessMessagesCallback;





public:
    // For Chat
    bool sendChatMessage(
        const QString& nByteSender_,
        const QString& nByteReceiver,
        const QString& strChatText_);
    void setChatCallback(std::function<void(int nType_)> nFun_)
    {
        m_nChatCallback = nFun_;
    }

private:
    bool PackChatMessage(
        const QString& nByteSender_,
        const QString& nByteReceiver,
        const QString& strChatText_,
        Buffer& nBuf_);

private:
    std::function<void(int nType_)> m_nChatCallback;

public:
    // for update friend list
    bool sendUpdateFriendListMessage(const QString& strUserId_);
    void setUpdateFriendListCallback(std::function<void(const NDataStruct::DynArray<QByteArray>&)> nFun_)
    {
        m_nUpdateFriendListCallback = nFun_;
    }

private:
    bool PackUpdateFriendListMessage(const QString& strUserId_, Buffer& nBuf_);

private:
    std::function<void(const NDataStruct::DynArray<QByteArray>&)> m_nUpdateFriendListCallback;


public:
    // for need-to-process-user list
    bool sendNeedToProcessUserListMessage(const QString& strUserId_);
    void setNeedToProcessUserListCallback(std::function<void(const NDataStruct::DynArray<QByteArray>&)> nFun_)
    {
        m_nNeedToProcessUserListCallback = nFun_;
    }

private:
    bool PackNeedToProcessUserListMessage(const QString& strUserId_, Buffer& nBuf_);

private:
    std::function<void(const NDataStruct::DynArray<QByteArray>&)> m_nNeedToProcessUserListCallback;


public:
    void setNewMessageCallback(std::function<void(const QByteArray&)> nFun_)
    {
        m_nNewMessageCallback = nFun_;
    }

private:
    std::function<void(const QByteArray&)> m_nNewMessageCallback;



public:
    // for off-line notification
    bool sendOffLineMessage(const QString& strUserId_, const TimeStamp& nTimeStamp_);

private:
    bool PackOffLineMessage(const QString& strUserId_, const TimeStamp& nTimeStamp_, Buffer& nBuf_);


public:
    // For Login
    bool sendLoginMessage(const QString& strUserId_, const QString& strPassword_);
    void setLoginCallback(std::function<void(int nType_, TimeStamp nTimeStamp_)> nFun_)
    {
        m_nLoginCallback = nFun_;
    }

private:
    bool PackLoginMessage(const QString& strUserId_, const QString& strPassword_, Buffer& nBuf_);

private:
    std::function<void(int nType_, TimeStamp nTimeStamp_)> m_nLoginCallback;

public:
    // For Add
    bool sendAddMessage(const QString& strMasterUserId_, const QString& strUserId_);
    void setAddCallback(std::function<void(int nType_)> nFun_)
    {
        m_nAddCallback = nFun_;
    }

private:
    bool PackAddMessage(const QString& strMasterUserId_, const QString& strUserId_, Buffer& nBuf_);

private:
    std::function<void(int nType_)> m_nAddCallback;

public:
    static MuduoClient* instance(EventLoop* loop, const InetAddress& serverAddr);
    void connect();
    void disconnect();
    //void write(const StringPiece& message);

private:
    void onConnection(const TcpConnectionPtr& conn);
    void onStringMessage(
          const TcpConnectionPtr&,
          const string& message,
          TimeStamp);

    void onMessage(
          const TcpConnectionPtr& conn,
          Buffer* buf,
          TimeStamp receiveTime);
    void onStringMessage(
        const TcpConnectionPtr&,
        char* strMessage_,
        int nLength_,
        TimeStamp);
private:
    MuduoClient(EventLoop* loop, const InetAddress& serverAddr);

private:
    static MuduoClient* __instance;

private:
    TcpClient m_nClient;
    //LengthHeaderCodec m_nCodec;
    MutexLock m_nMutex;
    TcpConnectionPtr m_nConnection;
};

#endif // MUDUOCLIENT_H
