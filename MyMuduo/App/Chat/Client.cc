#include "../../Tcp/lib.h"
#include "codec.h"
#include <iostream>
/*int fd = -1;
void LogOutput(const char* msg, int len)
{
  assert(fd != -1);
  size_t n = write(fd, msg, len);
  fsync(fd);
  (void)n;
}*/

class ChatClient 
{
 public:
  ChatClient(EventLoop* loop, const InetAddress& serverAddr)
    : m_nClient(loop, serverAddr, "ChatClient"),
      m_nCodec(std::bind(
                  &ChatClient::onStringMessage, 
                  this, 
                  _1, 
                  _2, 
                  _3))
  {
    m_nClient.setConnectionCallback(
        std::bind(
            &ChatClient::onConnection, 
            this, 
            _1));

    // 作为客户端的用户通过提供的接口来使用
    // 至于内部一系列流程的正确处理和高效运作属于网络库的责任
    m_nClient.setMessageCallback(
        std::bind(
            &LengthHeaderCodec::onMessage, 
            &m_nCodec, 
            _1, 
            _2, 
            _3));
    m_nClient.enableRetry();
  }

  void connect()
  {
    m_nClient.connect();
  }

  void disconnect()
  {
    m_nClient.disconnect();
  }

  // TcpClient本身不提供写如套接字接口
  // 可能是写入消息本身有一个协议编码过程
  // 原始信息－－协议编码－－编码后信息写入交给应用层处理
  // 利用TcpConnection对象可写入传入信息
  void write(const StringPiece& message)
  {
    MutexLockGuard lock(m_nMutex);
    if (m_nConnection)
    {
      m_nCodec.send(
              get_pointer(m_nConnection), 
              message);
    }
  }

 private:
  void onConnection(
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


  void onStringMessage(
          const TcpConnectionPtr&,
          const string& message,
          TimeStamp)
  {
    printf(
            "<<< %s\n", 
            message.c_str());
  }

private:
  TcpClient m_nClient;
  LengthHeaderCodec m_nCodec;
  MutexLock m_nMutex;
  TcpConnectionPtr m_nConnection;
};


// 使用客户端的接口：
// １．启动连接
// ２．断开连接
// ３．设置属性
// ４．向连接套接字写入内容
// ５．从连接套接字获得收到的消息
/*int main(int argc, char* argv[])
{
  fd = open(argv[3], O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);  
  Logger::setOutput(LogOutput);
  LOG_TRACE 
      << "pid = " 
      << getpid();
  if (argc > 2)
  {
    EventLoopThread loopThread;
    uint16_t port = (uint16_t)(atoi(argv[2]));
    InetAddress serverAddr(argv[1], port);
    // 客户端连接发起＋已经连接套接字处理放在一个线程
    ChatClient client(
            loopThread.startLoop(),  
            serverAddr);
    client.connect();

    // 获取终端输入，写入客户端放在一个线程
    std::string line;
    while (std::getline(std::cin, line))
    {
      client.write(line);
    }

    client.disconnect();
    sleepUsec(1000*1000);  
  }
  else
  {
    printf(
            "Usage: %s host_ip port\n", 
            argv[0]);
  }
}*/


