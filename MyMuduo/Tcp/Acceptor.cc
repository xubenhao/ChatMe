#include "Acceptor.h"

Acceptor::Acceptor(
    EventLoop* loop, 
    const InetAddress& listenAddr,
    bool reuseport)
  : m_pLoop(loop),
    // 创建IPV4+TCP套接字
    // 该套接字非阻塞＋执行exec时关闭
    m_nAcceptSocket(
        createNonblockingOrDie(listenAddr.family())),
    m_nAcceptChannel(
        m_pLoop, 
        m_nAcceptSocket.fd()),
    m_bListenning(false),
    m_nIdleFd(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
  assert(m_nIdleFd >= 0);
  // 处理＆设置套接字及其选项＆属性
  // &在非阻塞套接字上发起连接／关闭等请求
  m_nAcceptSocket.setReuseAddr(true);
  m_nAcceptSocket.setReusePort(reuseport);
  m_nAcceptSocket.bindAddress(listenAddr);
  
  // 粘合事件循环＋套接字
  m_nAcceptChannel.setReadCallback(
      std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
  m_nAcceptChannel.disableAll();
  m_nAcceptChannel.remove();
  close(m_nIdleFd);
}

void Acceptor::listen()
{
  m_pLoop->assertInLoopThread();
  m_bListenning = true;
  m_nAcceptSocket.listen();
  
  // 套接字上有很多事件，
  // 有些需要处理，有些不需要
  // enableReading会向
  // AcceptChannel关联的事件循环
  // 注册Channel
  // 让事件循环通过Poll检测
  // 此Channel所含描述符的可读事件
  //
  //
  // 向关联的事件循环内的Poll注册监听描述符的可读事件
  //
  m_nAcceptChannel.enableReading();
}

// 对非阻塞的发起了监听的套接字
// 在其关联的已经连接队列非空时，
// 该套接字变得可读
void Acceptor::handleRead()
{
  m_pLoop->assertInLoopThread();
  InetAddress peerAddr;
  // 从监听套接字已经连接队列取出一个已经连接套接字
  int connfd = m_nAcceptSocket.accept(&peerAddr);
  if (connfd >= 0)
  {
    if (m_nNewConnectionCallback)
    {
      m_nNewConnectionCallback(connfd, peerAddr);
    }
    else
    {
      close(connfd);
    }
  }
  else
  {
    LOG_SYSERR << "in Acceptor::handleRead";
    if (errno == EMFILE)
    {
      ::close(m_nIdleFd);
      m_nIdleFd = ::accept(m_nAcceptSocket.fd(), NULL, NULL);
      ::close(m_nIdleFd);
      m_nIdleFd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}
