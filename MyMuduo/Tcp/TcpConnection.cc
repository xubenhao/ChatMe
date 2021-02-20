#include "TcpConnection.h"

void defaultConnectionCallback(
    const TcpConnectionPtr& conn)
{
  LOG_INFO 
      << conn->localAddress().toIpPort() 
      << " -> "
      << conn->peerAddress().toIpPort() 
      << " is "
      << (conn->connected() ? "UP" : "DOWN");
}

void defaultMessageCallback(
    const TcpConnectionPtr&,
    Buffer* buf,
    TimeStamp)
{
  buf->retrieveAll();
}

TcpConnection::TcpConnection(
    EventLoop* loop,
    const string& nameArg,
    int sockfd,
    const InetAddress& localAddr,
    const InetAddress& peerAddr)
  : m_pLoop(CHECK_NOTNULL(loop)),
    m_strName(nameArg),
    m_nState(s_nConnecting),
    m_bReading(true),
    m_pSocket(new Socket(sockfd)),
    m_pChannel(new Channel(loop, sockfd)),
    m_nLocalAddr(localAddr),
    m_nPeerAddr(peerAddr),
    m_nHighWaterMark(64*1024*1024)
{
  m_pChannel->setReadCallback(
      std::bind(
          &TcpConnection::handleRead, 
          this, 
          _1));
  m_pChannel->setWriteCallback(
      std::bind(
          &TcpConnection::handleWrite, 
          this));
  m_pChannel->setCloseCallback(
      std::bind(
          &TcpConnection::handleClose, 
          this));
  m_pChannel->setErrorCallback(
      std::bind(
          &TcpConnection::handleError, 
          this));
  LOG_DEBUG 
      << "TcpConnection::ctor[" 
      <<  m_strName 
      << "] at " 
      << this
      << " fd=" 
      << sockfd;
  
  m_pSocket->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
  LOG_DEBUG 
      << "TcpConnection::dtor[" 
      <<  m_strName 
      << "] at " 
      << this
      << " fd=" 
      << m_pChannel->fd()
      << " state=" 
      << stateToString();
  assert(m_nState == s_nDisconnected);
}

bool TcpConnection::getTcpInfo(
        struct tcp_info* tcpi) const
{
  return m_pSocket->getTcpInfo(tcpi);
}

string TcpConnection::getTcpInfoString() const
{
  char buf[1024];
  buf[0] = '\0';
  m_pSocket->getTcpInfoString(buf, sizeof buf);
  return buf;
}

void TcpConnection::send(
        const void* data, 
        int len)
{
  send(StringPiece((const char*)(data), len));
}

void TcpConnection::send(
        const StringPiece& message)
{
  
  if (m_nState == s_nConnected)
  {
    if (m_pLoop->isInLoopThread())
    {
      sendInLoop(message);
    }
    else
    {
        void (TcpConnection::*fp)(const StringPiece& message) = 
          &TcpConnection::sendInLoop;
        m_pLoop->runInLoop(
          std::bind(
            fp,
            this,     
            message.as_string()));
    }
  }
}

void TcpConnection::send(
        Buffer* buf)
{
  if (m_nState == s_nConnected)
  {
    if (m_pLoop->isInLoopThread())
    {
        LOG_INFO 
            << "执行发送的线程是连接套接字事件循环所在的线程";
        sendInLoop(
              buf->peek(), 
              buf->readableBytes());
        buf->retrieveAll();
    }
    else
    {
      void (TcpConnection::*fp)(const StringPiece& message) = 
          &TcpConnection::sendInLoop;
      LOG_INFO 
          << "执行发送的线程不是连接套接字事件循环所在的线程";
      m_pLoop->runInLoop(
          std::bind(
              fp,
              this,     
              buf->retrieveAllAsString()));
    }
  }
}

void TcpConnection::sendInLoop(
        const StringPiece& message)
{
  sendInLoop(
          message.data(), 
          message.size());
}


void TcpConnection::sendInLoop(
        const void* data, 
        size_t len)
{
  m_pLoop->assertInLoopThread();
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool faultError = false;
  if (m_nState == s_nDisconnected)
  {
    LOG_WARN 
        << "disconnected, give up writing";
    return;
  }
    
  if (!m_pChannel->isWriting() 
          && m_nOutputBuffer.readableBytes() == 0)
  {
    nwrote = write(
            m_pChannel->fd(), 
            data, 
            len);
    if (nwrote >= 0)
    {
      remaining = len - nwrote;
      if (remaining == 0 
              && m_nWriteCompleteCallback)
      {
        m_pLoop->queueInLoop(
            std::bind(
                m_nWriteCompleteCallback, 
                shared_from_this()));
      }
    }
    else 
    {
      nwrote = 0;
      if (errno != EWOULDBLOCK)
      {
        LOG_SYSERR << "TcpConnection::sendInLoop";
        if (errno == EPIPE || errno == ECONNRESET) 
        {
          faultError = true;
        }
      }
    }
  }

  assert(remaining <= len);
  if (!faultError 
          && remaining > 0)
  {
    size_t oldLen = m_nOutputBuffer.readableBytes();
    if (oldLen + remaining >= m_nHighWaterMark
        && oldLen < m_nHighWaterMark
        && m_nHighWaterMarkCallback)
    {
      // 此回调意义在于通知当前排队等待写入的数据量达到了一定的数量级
      m_pLoop->queueInLoop(
        std::bind(
            m_nHighWaterMarkCallback, 
            shared_from_this(), 
            oldLen + remaining));
    }

    m_nOutputBuffer.append(
           (const char*)(data)+nwrote, remaining);
    if (!m_pChannel->isWriting())
    {
      m_pChannel->enableWriting();
    }
  }
}

void TcpConnection::shutdown()
{
  if (m_nState == s_nConnected)
  {
    setState(s_nDisconnecting);
    m_pLoop->runInLoop(
            std::bind(
                &TcpConnection::shutdownInLoop, 
                this));
  }
}

// 发起关闭时，默认关闭向对端的写
// 若此时有等待发到对端的数据尚未发送
// 先把数据都发送完毕，
// 再执行写关闭．
// 典型的事件处理＋非阻塞I/O下将一个连贯任务切分成多个阶段的编程模式
// 每次处理事件时，需要借助状态信息，来完成当前阶段的处理，
// 并视需要启动下一阶段
void TcpConnection::shutdownInLoop()
{
  m_pLoop->assertInLoopThread();
  if (!m_pChannel->isWriting())
  {
    m_pSocket->shutdownWrite();
  }
}

void TcpConnection::forceClose()
{
  if (m_nState == s_nConnected 
    || m_nState == s_nDisconnecting)
  {
    setState(s_nDisconnecting);
    m_pLoop->queueInLoop(
        std::bind(
            &TcpConnection::forceCloseInLoop, 
            shared_from_this()));
  }
}

void TcpConnection::forceCloseWithDelay(double seconds)
{
  if (m_nState == s_nConnected 
    || m_nState == s_nDisconnecting)
  {
    setState(s_nDisconnecting);
    m_pLoop->runAfter(
        seconds,
        makeWeakCallback(shared_from_this(),
        &TcpConnection::forceClose));  
  }
}

void TcpConnection::forceCloseInLoop()
{
  m_pLoop->assertInLoopThread();
  if (m_nState == s_nConnected 
    || m_nState == s_nDisconnecting)
  {
    handleClose();
  }
}

const char* TcpConnection::stateToString() const
{
  switch (m_nState)
  {
    case s_nDisconnected:
      return "kDisconnected";
    case s_nConnecting:
      return "kConnecting";
    case s_nConnected:
      return "kConnected";
    case s_nDisconnecting:
      return "kDisconnecting";
    default:
      return "unknown state";
  }
}

void TcpConnection::setTcpNoDelay(bool on)
{
  m_pSocket->setTcpNoDelay(on);
}

void TcpConnection::startRead()
{
  m_pLoop->runInLoop(
    std::bind(&TcpConnection::startReadInLoop, 
        this));
}

void TcpConnection::startReadInLoop()
{
  m_pLoop->assertInLoopThread();
  if (!m_bReading || !m_pChannel->isReading())
  {
    m_pChannel->enableReading();
    m_bReading = true;
  }
}

void TcpConnection::stopRead()
{
  m_pLoop->runInLoop(
    std::bind(&TcpConnection::stopReadInLoop, 
        this));
}

void TcpConnection::stopReadInLoop()
{
  m_pLoop->assertInLoopThread();
  if (m_bReading || m_pChannel->isReading())
  {
    m_pChannel->disableReading();
    m_bReading = false;
  }
}

void TcpConnection::connectEstablished()
{
     LOG_INFO 
         << "we can process the connected socket's readable event";
     m_pLoop->assertInLoopThread();
    assert(m_nState == s_nConnecting);
    setState(s_nConnected);
    m_pChannel->tie(
          shared_from_this());
    m_pChannel->enableReading();
    
    // shared_from_this()
    // 返回的是一个智能指针对象
    // 该对象的底层对象即为this所指向的那个对象
    //
    //
    // shared_from_this返回一个shared_ptr<T>
    // 若通过this->fun() 
    // 内部执行shared_from_this不允许，触发异常
    // 若通过shared_ptr<T>->fun() 
    // 返回的shared_ptr<T>和shared_ptr<T>触发者
    // 有一致的底层对象，引用计数对象，且使得引用计数对象+1
    //
    //
    // 混用原始指针和shared_ptr是不好的用法，易于产生混乱
    // 为了实现杜绝原始指针，全部都是shared_ptr/weak_ptr/unique_ptr
    // 有了
    // enable_shared_from_this<T>&shared_from_this()
    m_nConnectionCallback(
            shared_from_this());
}

void TcpConnection::connectDestroyed()
{
  m_pLoop->assertInLoopThread();
  if (m_nState == s_nConnected)
  {
    setState(s_nDisconnected);
    m_pChannel->disableAll();
    m_nConnectionCallback(shared_from_this());
  }

  m_pChannel->remove();
}

void TcpConnection::handleRead(
        TimeStamp receiveTime)
{
    m_pLoop->assertInLoopThread();
  int savedErrno = 0;
  // 套接字读入内容放入输入缓冲区
  // 输入缓冲区分为三部分
  // 已经取出的读入区域
  // 可以取出的读入区域
  // 可以放入新读入区域
  ssize_t n = m_nInputBuffer.readFd(
          m_pChannel->fd(), 
          &savedErrno);
  LOG_INFO 
      << "handleRead:" 
      << n;
  if (n > 0)
  {
      // 已经连接字可读的字节流先读入套接字缓冲区
      // 再触发应用层回调
      // 回调处理时，
      // 使用者可以按传输协议对内容进行分析，
      // 决定是否取出，还是放弃，待后续数据完整时再处理
      m_nMessageCallback(
            shared_from_this(), 
            &m_nInputBuffer, 
            receiveTime);
  }
  else if (n == 0)
  {
    LOG_INFO 
        << "read 0,when receive peer's fin"; 
    handleClose();
  }
  else
  {
    LOG_INFO 
        << "read -1,when receive peer's rst";
    errno = savedErrno;
    LOG_SYSERR << "TcpConnection::handleRead";
    handleError();
  }
}

void TcpConnection::handleWrite()
{
  m_pLoop->assertInLoopThread();
  if (m_pChannel->isWriting())
  {
    ssize_t n = write(
            m_pChannel->fd(),
            m_nOutputBuffer.peek(),
            m_nOutputBuffer.readableBytes());
    if (n > 0)
    {
      m_nOutputBuffer.retrieve(n);
      // readableBytes为０表示所有可写的都写完了
      if (m_nOutputBuffer.readableBytes() == 0)
      {
          // 因为此时没有数据需排队写到套接字
          // 不用对其开启可写监视
        m_pChannel->disableWriting();
        // 指向写完回调
        if (m_nWriteCompleteCallback)
        {
          m_pLoop->queueInLoop(
            std::bind(
                m_nWriteCompleteCallback, 
                shared_from_this()));
        }

        if (m_nState == s_nDisconnecting)
        {
          shutdownInLoop();
        }
      }
    }
    else
    {
      LOG_SYSERR 
          << "TcpConnection::handleWrite";
    }
  }
  else
  {
    LOG_INFO 
        << "Connection fd = " 
        << m_pChannel->fd()
        << " is down, no more writing";
  }
}

void TcpConnection::handleClose()
{
  m_pLoop->assertInLoopThread();
  LOG_INFO 
      << "fd = " 
      << m_pChannel->fd() 
      << " state = " 
      << stateToString();
  // 若再此之前本地已经关闭了向对端的写
  // 则此时为s_nDisconnecting否则为s_nConnected
  assert(m_nState == s_nConnected 
          || m_nState == s_nDisconnecting);
  // 对端的Fin已经收到，本端也执行关闭
  setState(s_nDisconnected);
  // 会从事件循环的Poll里删除对描述符的监视
  m_pChannel->disableAll();

  // 这里需要一个shared_ptr<TcpConnection>的原因
  // 执行m_nConnectionCallback在关闭时，
  // 目前对服务器，会在此事件回调中删除
  // shared_ptr<TcpConnection>从而可能导致
  // 后续执行时，此事件处理调用隐含的this指向一个已被销毁的对象
  // 这里产生一个临时shared_ptr<...>
  // 保证到此事件处理结束时，this指向的对象都是有效的
  TcpConnectionPtr guardThis(shared_from_this());
 
  // 关于c++对象管理
  // c++一切结尾对象［基础数据类型也可视为对象］
  // 对象分为两种类型
  // 值类型对象
  // 引用类型对象
  //
  // 特点：
  // 值类型对象支持拷贝构造和赋值
  // 使用上不通过指针使用对象
  // 直接使用对象本身，对象引用来使用
  //
  // 特点：
  // 引用类型对象不支持拷贝构造和赋值
  // 使用引用类型对象基本通过指针来使用
  //
  // 对象管理方式
  // 仅仅针对引用类型的对象需要进行管理
  // 值类型对象因为直接使用对象本身，
  // 对象在离开其作用域，自动销毁，释放空间．
  // 不需额外管理．
  //
  // 对象管理：
  // 方式１：
  // 每个对象有一个唯一的拥有者
  // 每个对象可以拥有０个或多个对象
  // 每个对象销毁时，
  // 先销毁其拥有的每个孩子，再销毁自身
  //
  // 这样，通过为对象设置拥有者，
  // 对象管理的责任就交给了对象拥有者
  // 而对象使用者，只需使用，不用对其管理
  //
  // 对拥有者不存在的对象，
  // 则使用完毕时，可以通过delete来销毁它．
  //
  //
  // 方式２：
  // 通过智能指针
  // 要使用智能指针管理对象
  // 则在用new 创建对象初始化智能指针后
  // 后续所有关于此对象的使用必须全部用智能指针
  // 而不能用原始指针
  // shared_from_this
  // 在一个类型为引用类型时，且希望通过智能指针进行资源管理时，
  // 类型需继承此模板类型
  m_nConnectionCallback(guardThis);
  m_nCloseCallback(guardThis);
}

void TcpConnection::handleError()
{
  int err = getSocketError(m_pChannel->fd());
  LOG_ERROR 
      << "TcpConnection::handleError [" 
      << m_strName
      << "] - SO_ERROR = " 
      << err 
      << " " 
      << strerror_tl(err);
  if(m_nErrCallback)
  {
    m_nErrCallback(shared_from_this());
  }

}

