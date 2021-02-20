//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#include "muduoclient.h"

void LogOutput(const char* msg, int len)
{
  static int fd = -1;
  if(fd == -1)
  {
      fd = open("log.txt", O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
  }

  write(fd, msg, len);
  fsync(fd);
}

MuduoClient* MuduoClient::__instance = nullptr;

MuduoClient* MuduoClient::instance(EventLoop* loop, const InetAddress& serverAddr)
{
    if(__instance == nullptr)
    {
        __instance = new MuduoClient(loop, serverAddr);
    }

    return __instance;
}

MuduoClient::MuduoClient(EventLoop* loop, const InetAddress& serverAddr)
    : m_nClient(loop, serverAddr, "MuduoClient"),// one tcp client
      m_nCodec(std::bind(
                  &MuduoClient::onStringMessage,
                  this,
                  _1,
                  _2,
                  _3))// for code and de-code
{
    // set client connection's building and destroying callback
    m_nClient.setConnectionCallback(
        std::bind(
            &MuduoClient::onConnection,
            this,
            _1));

    // set client connection's receiving orginal message callback
    m_nClient.setMessageCallback(
        std::bind(
            &LengthHeaderCodec::onMessage,
            &m_nCodec,
            _1,
            _2,
            _3));
    // set tcp client's property to enable retry
    m_nClient.enableRetry();
}

// send connecting request to server and start watching the event of readable
void MuduoClient::connect()
{
    m_nClient.connect();
}

// send fin to server to end connection
void MuduoClient::disconnect()
{
    m_nClient.disconnect();
}

// TcpClient本身不提供写如套接字接口
// 可能是写入消息本身有一个协议编码过程
// 原始信息－－协议编码－－编码后信息写入交给应用层处理
// 利用TcpConnection对象可写入传入信息

// the parameter is the origin message to write
// it should be coded before actually writed to server
void MuduoClient::write(const StringPiece& message)
{
    MutexLockGuard lock(m_nMutex);
    if (m_nConnection)
    {
      m_nCodec.send(
          get_pointer(m_nConnection),
          message);
    }
}

// the time of connection build and destroy
void MuduoClient::onConnection(
      const TcpConnectionPtr& conn)
{
    LOG_TRACE
        << conn->localAddress().toIpPort()
        << " -> "
        << conn->peerAddress().toIpPort()
        << " is "
        << (conn->connected() ? "UP" : "DOWN");

    MutexLockGuard lock(m_nMutex);
    if (conn->connected())
    {
        m_nConnection = conn;
    }
    else
    {
        m_nConnection.reset();
    }
}

// this is called when there have a completed message to process
// the parameter 2 is the message content.
void MuduoClient::onStringMessage(
      const TcpConnectionPtr&,
      const string& message,
      TimeStamp)
{
    printf(
        "<<< %s\n",
        message.c_str());
}
