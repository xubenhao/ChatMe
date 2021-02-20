#include "../../Lib/lib.h"
#include "../../Tcp/lib.h"
#include <queue>

const int s_nMaxConns = 10;  
const size_t s_nMaxPacketLen = 255;
const size_t s_nHeaderLen = 3;

const uint16_t s_nClientPort = 3333;
const char* backendIp = "127.0.0.1";
const uint16_t s_nBackendPort = 9999;

class MultiplexServer
{
 public:
  MultiplexServer(
    EventLoop* loop,
    const InetAddress& listenAddr,
    const InetAddress& backendAddr,
    int numThreads)
    : m_nServer(
        loop, 
        listenAddr, 
        "MultiplexServer"),
      m_nBackend(
        loop, 
        backendAddr, 
        "MultiplexBackend"),

      m_nNumThreads(numThreads),
      m_nOldCounter(0),
      m_nStartTime(TimeStamp::now())
  {
      // 服务器提供的接口中和回调有关的包含
      // 与客户端连接建立／销毁的回调：一般在此回调中存储传入的shared_ptr<TcpConnection>
      // 收到客户端消息时回调：一般在此回调中分析Buffer，看是否有一条完整消息，做出信息提取与处理／忽略．
      //
    m_nServer.setConnectionCallback(
        std::bind(
            &MultiplexServer::onClientConnection, 
            this, 
            _1));
    m_nServer.setMessageCallback(
        std::bind(
            &MultiplexServer::onClientMessage, 
            this, 
            _1, 
            _2, 
            _3));
    m_nServer.setThreadNum(numThreads);

    // 客户端提供的接口中和回调有关的包含
    // 与服务器连接建立／销毁的回调：一般在此回调中存储传入的shared_ptr<TcpConnection>
    // 收到服务器消息时回调：一般在此回调中分析Buffer，看是否有一条完整消息，做出信息提取与处理／忽略
    //
    //
    m_nBackend.setConnectionCallback(
        std::bind(
            &MultiplexServer::onBackendConnection, 
            this, 
            _1));
    m_nBackend.setMessageCallback(
        std::bind(
            &MultiplexServer::onBackendMessage, 
            this, 
            _1, 
            _2, 
            _3));
    m_nBackend.enableRetry();

    // 我们向某个已连接套接字写入数据，断开和对端的连接一般都是通过TcpConnection提供的接口来完成的
    //
    // loop->runEvery(
    //  10.0, 
    //  std::bind(
    //      &MultiplexServer::printStatistics, 
    //      this));
  }

  // 启动：
  // 作为服务器启动监听
  // 作为客户端启动连接
  void start()
  {
    LOG_INFO 
        << "starting " 
        << m_nNumThreads 
        << " threads.";
    // 同时启动客户端和服务器
    // 服务器在本机3333监听连接请求
    // 客户端向指定ip的9999发起连接
    m_nBackend.connect();
    m_nServer.start();
  }

private:
  // 常规操作－－连接建立时，存储shared_ptr<TcpConnection>
  // 连接销毁时－－删除存储的shared_ptr<TcpConnection>
  // 要向客户端写入消息／关闭与其连接时，通过TcpConnection提供的接口完成
  void onClientConnection(
    const TcpConnectionPtr& conn)
  {
      // TCP已经连接套接字可同时取得对端和本端信息
    LOG_TRACE 
        << "Client " 
        << conn->peerAddress().toIpPort() 
        << " -> "
        << conn->localAddress().toIpPort() 
        << " is "
        << (conn->connected() ? "UP" : "DOWN");

    // 连接建立
    // １．分配id
    // ２．存储shared_ptr<TcpConnection>
    // ３．sendBackendString
    //
    // 每个TcpConnection对象本身允许绑定一个任意类型的自定义数据
    // 可以把id作为其语境与其绑定
    // 这样可以很方便的实现，id到shared_ptr<TcpConnection>，TcpConnection到id的双向查找
    if (conn->connected())
    {
      int id = -1;
      {
          // 每个客户端分配一个id
        MutexLockGuard lock(m_nMutex);
        if (!m_nAvailIds.empty())
        {
            // 监听到一个新连接
            // 充当消费者
          id = m_nAvailIds.front();
          m_nAvailIds.pop();
          // 消费一个m_nAvailIds头部的id
          // 建立映射－－id到shared_ptr<TcpConnection>
          m_nClientConns[id] = conn;
        }
      }

      if (id <= 0)
      {
        conn->shutdown();
      }
      else
      {
        conn->setContext(id);
        char buf[256];
        snprintf(
            buf, 
            sizeof(buf), 
            "CONN %d FROM %s IS UP\r\n", 
            id,
            conn->peerAddress().toIpPort().c_str());
        sendBackendString(0, buf);
      }
    }
    // 连接断开
    // １．回收id
    // ２．删除保存的shared_ptr<TcpConnection>
    // ３．sendBackendString
    else
    {
      if (!conn->getContext().empty())
      {
        int id = boost::any_cast<int>(conn->getContext());
        assert(id > 0 && id <= s_nMaxConns);
        char buf[256];
        snprintf(
            buf, 
            sizeof(buf), 
            "CONN %d FROM %s IS DOWN\r\n",
            id, 
            conn->peerAddress().toIpPort().c_str());
        sendBackendString(
            0, 
            buf);

        MutexLockGuard lock(m_nMutex);
        if (m_nBackendConn)
        {
          m_nAvailIds.push(id);
          m_nClientConns.erase(id);
        }
        else
        {
          assert(m_nAvailIds.empty());
          assert(m_nClientConns.empty());
        }
      }
    }
  }

  // sendBackendBuffer(客户id, buf)
  void onClientMessage(
    const TcpConnectionPtr& conn, 
    Buffer* buf, 
    TimeStamp)
  {
    size_t len = buf->readableBytes();
    m_nTransferred.addAndGet(len);
    m_nReceivedMessages.incrementAndGet();
    if (!conn->getContext().empty())
    {
      int id = boost::any_cast<int>(conn->getContext());
      sendBackendBuffer(id, buf);
    }
    else
    {
      buf->retrieveAll();
    }
  }

  // 作为客户端与服务器取得连接时，
  // １．初始化id池［id池初始化后，才能作为服务器让到来的连接被接收和被分配id］
  // ２．记录shared_ptr<TcpConnection>
  //
  // 作为客户端与服务器断开连接时
  // １．删除保存的shared_ptr<TcpConnection>
  // ２．清空id池可用部分
  // ３．对所有服务的客户端让其关闭
  void onBackendConnection(
    const TcpConnectionPtr& conn)
  {
    LOG_TRACE 
        << "Backend " 
        << conn->localAddress().toIpPort() 
        << " -> "
        << conn->peerAddress().toIpPort() 
        << " is "
        << (conn->connected() ? "UP" : "DOWN");
    std::vector<TcpConnectionPtr> connsToDestroy;
    if (conn->connected())
    {
      MutexLockGuard lock(m_nMutex);
      m_nBackendConn = conn;
      assert(m_nAvailIds.empty());
      for (int i = 1; i <= s_nMaxConns; ++i)
      {
        m_nAvailIds.push(i);
      }
    }
    else
    {
      MutexLockGuard lock(m_nMutex);
      m_nBackendConn.reset();
      connsToDestroy.reserve(
        m_nClientConns.size());
      for (std::map<int, TcpConnectionPtr>::iterator it = m_nClientConns.begin();
          it != m_nClientConns.end();
          ++it)
      {
        connsToDestroy.push_back(it->second);
      }

      m_nClientConns.clear();
      while (!m_nAvailIds.empty())
      {
        m_nAvailIds.pop();
      }
    }

    for (std::vector<TcpConnectionPtr>::iterator it = connsToDestroy.begin();
        it != connsToDestroy.end();
        ++it)
    {
      (*it)->shutdown();
    }
  }

  // sendToClient
  void onBackendMessage(
    const TcpConnectionPtr& conn, 
    Buffer* buf, 
    TimeStamp)
  {
    size_t len = buf->readableBytes();
    m_nTransferred.addAndGet(len);
    m_nReceivedMessages.incrementAndGet();
    sendToClient(buf);
  }




  void sendBackendString(
    int id, 
    const string& msg)
  {
    assert(msg.size() <= s_nMaxPacketLen);
    Buffer buf;
    buf.append(msg);
    sendBackendPacket(
        id, 
        &buf);
  }

  void sendBackendBuffer(
    int id, 
    Buffer* buf)
  {
    while (buf->readableBytes() > s_nMaxPacketLen)
    {
      Buffer packet;
      packet.append(
        buf->peek(), 
        s_nMaxPacketLen);
      buf->retrieve(s_nMaxPacketLen);
      sendBackendPacket(
        id, 
        &packet);
    }

    if (buf->readableBytes() > 0)
    {
      sendBackendPacket(id, buf);
    }
  }


  void sendBackendPacket(
    int id, 
    Buffer* buf)// buf的数据部分不会超过设定限制｀
  {
    size_t len = buf->readableBytes();
    assert(len <= s_nMaxPacketLen);
    uint8_t header[s_nHeaderLen] = 
    {
      static_cast<uint8_t>(len),
      static_cast<uint8_t>(id & 0xFF),
      static_cast<uint8_t>((id & 0xFF00) >> 8)
    };
   
    // 消息头
    buf->prepend(
        header, 
        s_nHeaderLen);

    TcpConnectionPtr backendConn;
    {
      MutexLockGuard lock(m_nMutex);
      backendConn = m_nBackendConn;
    }

    // 发送：消息头＋消息数据
    if (backendConn)
    {
      m_nBackendConn->send(buf);
    }
  }

  
  

  // 作为客户端收到的来自服务器的信息
  void sendToClient(Buffer* buf)
  {
    while (buf->readableBytes() > s_nHeaderLen)
    {
      int len = static_cast<uint8_t>(*buf->peek());
      // 收到的消息不足一条完整的消息，处理策略是忽略
      // 收到的消息达到一条完整消息，
      // 取出消息处理．
      // 若还有消息，继续处理
      if (buf->readableBytes() < len + s_nHeaderLen)
      {
        break;
      }
      else
      {
        int id = static_cast<uint8_t>(buf->peek()[1]);
        id |= (static_cast<uint8_t>(buf->peek()[2]) << 8);
        TcpConnectionPtr clientConn;
        {
          MutexLockGuard lock(m_nMutex);
          std::map<int, TcpConnectionPtr>::iterator it = m_nClientConns.find(id);
          if (it != m_nClientConns.end())
          {
            clientConn = it->second;
          }
        }

        if (clientConn)
        {
          clientConn->send(
            buf->peek() + s_nHeaderLen, 
            len);
        }

        buf->retrieve(len + s_nHeaderLen);
      }
    }
  }

  
  void printStatistics()
  {
    TimeStamp endTime = TimeStamp::now();
    int64_t newCounter = m_nTransferred.get();
    int64_t bytes = newCounter - m_nOldCounter;
    int64_t msgs = m_nReceivedMessages.getAndSet(0);
    double time = timeDifference(endTime, m_nStartTime);
    printf(
        "%4.3f MiB/s %4.3f Ki Msgs/s %6.2f bytes per msg\n",
        static_cast<double>(bytes)/time/1024/1024,
        static_cast<double>(msgs)/time/1024,
        static_cast<double>(bytes)/static_cast<double>(msgs));

    m_nOldCounter = newCounter;
    m_nStartTime = endTime;
  }

private:
  TcpServer m_nServer;
  TcpClient m_nBackend;
  int m_nNumThreads;
  
  AtomicInt64 m_nTransferred;
  AtomicInt64 m_nReceivedMessages;
  
  int64_t m_nOldCounter;
  TimeStamp m_nStartTime;
  
  MutexLock m_nMutex;
  // 存储作为客户端与服务器的TcpConnection
  TcpConnectionPtr m_nBackendConn;
  
  // 存储作为服务器所服务的客户连接
  std::map<int, TcpConnectionPtr> m_nClientConns;
  std::queue<int> m_nAvailIds;
};

/*int main(
    int argc, 
    char* argv[])
{
  LOG_INFO 
      << "pid = " 
      << getpid() 
      << ", tid = " 
      << tid();
  int numThreads = 4;
  if (argc > 1)
  {
    backendIp = argv[1];
  }

  if (argc > 2)
  {
    numThreads = atoi(argv[2]);
  }
  
  EventLoop loop;
  // 本机ip+3333
  InetAddress listenAddr(
    s_nClientPort);
  
  // 指定ip+9999
  InetAddress backendAddr(
    backendIp, 
    s_nBackendPort);
  MultiplexServer server(
    &loop, 
    listenAddr, // 监听3333
    backendAddr, // 指定ip+9999
    numThreads);// 服务器线程池

  server.start();
  loop.loop();
}*/


