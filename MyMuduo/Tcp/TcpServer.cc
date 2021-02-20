#include "TcpServer.h"

TcpServer::TcpServer(
    EventLoop* loop,
    const InetAddress& listenAddr,
    const string& nameArg,
    Option option)
  : m_pLoop(CHECK_NOTNULL(loop)),// 事件循环对象
    m_strIpPort(listenAddr.toIpPort()),// 端口
    m_strName(nameArg),// 名称
    m_pAcceptor(
        new Acceptor(
            loop, // 事件循环对象
            listenAddr, // 地址
            option == s_nReusePort)),
    m_pThreadPool(
            new EventLoopThreadPool(
                loop, // 事件循环对象
                m_strName)),// 名称
    m_nConnectionCallback(
            defaultConnectionCallback),
    m_nMessageCallback(
            defaultMessageCallback),
    m_nNextConnId(1)
{
    m_pAcceptor->setNewConnectionCallback(
      std::bind(
          &TcpServer::newConnection, 
          this,
          _1, 
          _2));
}

TcpServer::~TcpServer()
{
  LOG_INFO
      << "TcpServer dctor";
  m_pLoop->assertInLoopThread();
  LOG_INFO 
      << "TcpServer::~TcpServer [" 
      << m_strName 
      << "] destructing";
  
  for (auto& item : m_nConnections)
  {
    TcpConnectionPtr conn(item.second);
    item.second.reset();
    // 每个已经连接套接字占据一个线程用一个事件循环处理
    conn->getLoop()->runInLoop(
      std::bind(&TcpConnection::connectDestroyed, 
      conn));
  }
}

void TcpServer::setThreadNum(int numThreads)
{
  assert(0 <= numThreads);
  m_pThreadPool->setThreadNum(numThreads);
}

// 服务器的开始
void TcpServer::start()
{
  if (m_nStarted.getAndSet(1) == 0)
  {
    m_pThreadPool->start(
            m_nThreadInitCallback);

    assert(!m_pAcceptor->listenning());
    m_pLoop->runInLoop(
        std::bind(
            &Acceptor::listen, 
            get_pointer(m_pAcceptor)));
  }
}

void TcpServer::newConnection(
    int sockfd, 
    const InetAddress& peerAddr)
{
  LOG_INFO
      << "New Connector process in tcpserver";
  m_pLoop->assertInLoopThread();
  EventLoop* ioLoop = m_pThreadPool->getNextLoop();
  char buf[64];
  snprintf(
    buf, 
    sizeof buf, 
    "-%s#%d", 
    m_strIpPort.c_str(), 
    m_nNextConnId);
  ++m_nNextConnId;
  string connName = m_strName + buf;
  LOG_INFO 
      << "TcpServer::newConnection [" 
      << m_strName
      << "] - new connection [" 
      << connName
      << "] from " 
      << peerAddr.toIpPort();
  InetAddress localAddr(getLocalAddr(sockfd));
  TcpConnectionPtr conn(
    new TcpConnection(
        ioLoop, 
        connName, 
        sockfd, 
        localAddr, 
        peerAddr));
  m_nConnections[connName] = conn;
  conn->setConnectionCallback(
          m_nConnectionCallback);
  conn->setMessageCallback(
          m_nMessageCallback);
  conn->setWriteCompleteCallback(
          m_nWriteCompleteCallback);
  conn->setCloseCallback(
          std::bind(&TcpServer::removeConnection, 
          this, 
          _1));
  
  if(ioLoop == m_pLoop)
  {
    LOG_INFO
        << "process established connect in main thread";
  }
  else
  {
    LOG_INFO
        << "process established connect in one thread in thread pool";
  }
    
  ioLoop->runInLoop(
      std::bind(
          &TcpConnection::connectEstablished, 
          conn));
}

void TcpServer::removeConnection(
        const TcpConnectionPtr& conn)
{
  m_pLoop->runInLoop(
    std::bind(
        &TcpServer::removeConnectionInLoop, 
        this, 
        conn));
}

void TcpServer::removeConnectionInLoop(
        const TcpConnectionPtr& conn)
{
  m_pLoop->assertInLoopThread();
  LOG_INFO 
      << "TcpServer::removeConnectionInLoop [" 
      << m_strName
      << "] - connection " 
      << conn->name();
  size_t n = m_nConnections.erase(conn->name());
  (void)n;
  assert(n == 1);
  EventLoop* ioLoop = conn->getLoop();
  ioLoop->queueInLoop(
      std::bind(&TcpConnection::connectDestroyed, conn));
}
