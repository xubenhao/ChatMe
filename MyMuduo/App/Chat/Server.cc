#include "codec.h"
#include "../../Lib/lib.h"
#include "../../Tcp/lib.h"

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
class ChatServer 
{
 public:
  ChatServer(EventLoop* loop, const InetAddress& listenAddr)
  : m_nServer(loop, listenAddr, "ChatServer"),
    m_nCodec(std::bind(
                &ChatServer::onStringMessage, 
                this, 
                _1, 
                _2, 
                _3))
  {
    m_nServer.setConnectionCallback(
        std::bind(
            &ChatServer::onConnection, 
            this, 
            _1));
    m_nServer.setMessageCallback(
        std::bind(
            &LengthHeaderCodec::onMessage, 
            &m_nCodec, 
            _1, 
            _2, 
            _3));
  }

  void setThreadNum(int numThreads)
  {
    m_nServer.setThreadNum(numThreads);
  }

  void start()
  {
    m_nServer.setThreadInitCallback(
            std::bind(
                &ChatServer::threadInit,// 线程池内每个线程会执行 
                this, 
                _1));
    m_nServer.start();
  }

private:
  void threadInit(EventLoop* loop)
  {
    // LocalConnections模板类
    // 模板类型T为容器，容器元素是值语义的智能指针
    assert(LocalConnections::pointer() == NULL);
    // 执行结果
    // 针对调用线程产生一个动态容器对象，
    // 调用线程的线程特定数据LocalConnections::m_pValue指向此动态对象
    LocalConnections::instance();
    assert(LocalConnections::pointer() != NULL);
    MutexLockGuard lock(m_nMutex);
    // 一个事件循环背后是一个与其绑定的线程
    // 通过事件循环对象的访问可以获得
    // 其状态，属性，与其交互［也即与其背后所关联的线程交互］
    m_nLoops.insert(loop);
  }

  // 不论连接回调由服务器端那个线程触发
  // 执行的回调函数都是这个
  // 且由于成员函数性质，所有线程执行回调时，
  // 均可获得一个多个线程共享的回调语境－－－一个共享的ChatServer对象
  void onConnection(const TcpConnectionPtr& conn)
  {
    LOG_TRACE 
        << conn->localAddress().toIpPort() 
        << " -> "
        << conn->peerAddress().toIpPort() 
        << " is "
        << (conn->connected() ? "UP" : "DOWN");
    
    if (conn->connected())
    {
        // 让调用线程在其自己的特定的动态对象里
        // 存储由其负责的所有TcpConnection对象
        LocalConnections::instance().insert(conn);
    }
    else
    {
      LocalConnections::instance().erase(conn);
    }
  }

  // 传入是
  // 原始接收信息－－解码－－解码后一条完整信息
  void onStringMessage(
          const TcpConnectionPtr&,
          const string& message,
          TimeStamp)
  {
    EventLoop::Functor f = 
        std::bind(
                &ChatServer::distributeMessage, 
                this, 
                message);
    LOG_DEBUG;
    MutexLockGuard lock(m_nMutex);

    // m_nLoops存储了服务器端所有工作线程的EventLoop对象
    // 让服务器的每个工作线程都去执行回调
    for (std::set<EventLoop*>::iterator it = m_nLoops.begin();
        it != m_nLoops.end();
        ++it)
    {
      (*it)->queueInLoop(f);
    }

    LOG_DEBUG;
  }

  typedef std::set<TcpConnectionPtr> ConnectionList;
  // 服务器的每个线程都执行此回调
  void distributeMessage(
          const string& message)
  {
    LOG_DEBUG << "begin";
    // 每个线程对所有由自己负责的TcpConnection执行
    // 原始信息－－信息编码－－信息发送
    for (ConnectionList::iterator it = LocalConnections::instance().begin();
        it != LocalConnections::instance().end();
        ++it)
    {
      m_nCodec.send(
              get_pointer(*it), 
              message);
    }

    LOG_DEBUG << "end";
  }


private:
  TcpServer m_nServer;
  LengthHeaderCodec m_nCodec;
  
  typedef ThreadLocalSingleton<ConnectionList> LocalConnections;
  MutexLock m_nMutex;
  std::set<EventLoop*> m_nLoops;
};

// 这是一个广播服务器
// 将任一客户端的消息传播给所有客户端
/*int main(int argc, char* argv[])
{
  LOG_INFO 
      << "pid = " 
      << getpid();
  if (argc > 1)
  {
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress serverAddr(port);
    ChatServer server(
            &loop, 
            serverAddr);
    if (argc > 2)
    {
      server.setThreadNum(atoi(argv[2]));
    }

    server.start();
    loop.loop();
  }
  else
  {
    printf("Usage: %s port [thread_num]\n", argv[0]);
  }
}*/

