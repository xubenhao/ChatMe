#include "TcpClient.h"
#include "Connector.h"
void removeConnection(
    EventLoop* loop, 
    const TcpConnectionPtr& conn)
{
    loop->queueInLoop(
        std::bind(
            &TcpConnection::connectDestroyed, 
            conn));
}

void removeConnector(
        const ConnectorPtr& connector)
{
    
}

TcpClient::TcpClient(
    EventLoop* loop,
    const InetAddress& serverAddr,
    const string& nameArg)
  : m_pLoop(CHECK_NOTNULL(loop)),
    m_nConnector(
            new Connector(
                loop, 
                serverAddr)),
    m_strName(nameArg),
    m_nConnectionCallback(defaultConnectionCallback),
    m_nMessageCallback(defaultMessageCallback),
    m_bRetry(false),
    m_bConnect(true),
    m_nNextConnId(1)
{
    // 设置新连接回调
    m_nConnector->setNewConnectionCallback(
      std::bind(
          &TcpClient::newConnection, 
          this, 
          _1));

    LOG_INFO 
        << "TcpClient::TcpClient[" 
        << m_strName
        << "] - connector " 
        << get_pointer(m_nConnector);
}

TcpClient::~TcpClient()
{
  LOG_INFO 
      << "TcpClient::~TcpClient[" 
      << m_strName
      << "] - connector " 
      << get_pointer(m_nConnector);
  
  TcpConnectionPtr conn;
  bool unique = false;
  {
    MutexLockGuard lock(m_nMutex);
    unique = m_nConnection.unique();
    conn = m_nConnection;
  }

  if (conn)
  {
    assert(m_pLoop == conn->getLoop());
    CloseCallback cb = std::bind(
            &::removeConnection, 
            m_pLoop, 
            _1);
    m_pLoop->runInLoop(
        std::bind(
            &TcpConnection::setCloseCallback, 
            conn, 
            cb));
    if (unique)
    {
      conn->forceClose();
    }
  }
  else
  {
      
    m_nConnector->stop();
    m_pLoop->runAfter(
            1, 
            std::bind(
                &removeConnector, 
                m_nConnector));
  }
}


void TcpClient::connect()
{
  LOG_INFO 
      << "TcpClient::connect[" 
      << m_strName 
      << "] - connecting to "
      << m_nConnector->serverAddress().toIpPort();
  m_bConnect = true;
  m_nConnector->start();
}

void TcpClient::disconnect()
{
  m_bConnect = false;
  
  {
    MutexLockGuard lock(m_nMutex);
    if (m_nConnection)
    {
      m_nConnection->shutdown();
    }
  }
}

void TcpClient::stop()
{
  m_bConnect = false;
  m_nConnector->stop();
}

// 事件回调＋非阻塞I/O的方式
// 完成一个连贯的任务需要多次事件回调
// 且每次事件回调时需要直到此时的状态信息
void TcpClient::newConnection(int sockfd)
{
  m_pLoop->assertInLoopThread();
  InetAddress peerAddr(getPeerAddr(sockfd));
  char buf[32];
  snprintf(
    buf, 
    sizeof buf, 
    ":%s#%d", 
    peerAddr.toIpPort().c_str(), 
    m_nNextConnId);
  
  ++m_nNextConnId;
  string connName = m_strName + buf;
  InetAddress localAddr(getLocalAddr(sockfd));

  // 一个TcpClient建立了与目的端的连接后
  // 此时Connector对象便不再具有价值
  // 有价值的是TcpConnection对象
  TcpConnectionPtr conn(
        new TcpConnection(
        m_pLoop, 
        connName, 
        sockfd, 
        localAddr, 
        peerAddr));

  // TcpConnection构成了已经连接套接字描述符的语境
  // 其中包含的
  // Channel对象构成了该描述符与epoll交互时的语境
  // 其他对象构成了一些其他需要的语境
  conn->setConnectionCallback(
          m_nConnectionCallback);
  conn->setMessageCallback(
          m_nMessageCallback);
  conn->setWriteCompleteCallback(
          m_nWriteCompleteCallback);
  conn->setCloseCallback(
      std::bind(
          &TcpClient::removeConnection, 
          this, 
          _1)); 
  
  {
    MutexLockGuard lock(m_nMutex);
    m_nConnection = conn;
  }

  conn->connectEstablished();
}

void TcpClient::removeConnection(
        const TcpConnectionPtr& conn)
{
  m_pLoop->assertInLoopThread();
  assert(m_pLoop == conn->getLoop());

  {
    MutexLockGuard lock(m_nMutex);
    assert(m_nConnection == conn);
    m_nConnection.reset();
  }

  m_pLoop->queueInLoop(
    std::bind(&TcpConnection::connectDestroyed, 
    conn));
  if (m_bRetry && m_bConnect)
  {
    LOG_INFO 
        << "TcpClient::connect[" 
        << m_strName 
        << "] - Reconnecting to "
        << m_nConnector->serverAddress().toIpPort();
    m_nConnector->restart();
  }
}
