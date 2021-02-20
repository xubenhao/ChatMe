#include "Channel.h"
#include "Logging.h"

const int Channel::s_nNoneEvent = 0;
const int Channel::s_nReadEvent = POLLIN | POLLPRI;
const int Channel::s_nWriteEvent = POLLOUT;

Channel::Channel(
    EventLoop* pLoop_, 
    int nFd_)
  : m_pLoop(pLoop_),
    m_nFd(nFd_),
    m_nEvents(0),
    m_nRevents(0),
    m_nIndex(-1),
    m_bLogHup(true),
    m_bTied(false),
    m_bEventHandling(false),
    m_bAddedToLoop(false)
{
}

Channel::~Channel()
{
  if (m_pLoop->isInLoopThread())
  {
    assert(!m_pLoop->hasChannel(this));
  }
}

void Channel::tie(
        const std::shared_ptr<void>& obj)
{
  m_pTie = obj;
  m_bTied = true;
}

// 一个事件循环可管理多个Channel
// 一个Channel至多绑定到一个事件循环，一个描述符
void Channel::update()
{
  m_bAddedToLoop = true;
  // 完成向事件循环的Poll注册
  // 所含描述符的事件
  m_pLoop->updateChannel(this);
}

void Channel::remove()
{
  m_bAddedToLoop = false;
  m_pLoop->removeChannel(this);
}

void Channel::handleEvent(
        TimeStamp receiveTime)
{
  std::shared_ptr<void> guard;
  if (m_bTied)
  {
    // weak_ptr<T>.lock得到shared_ptr<T>
    // shared_ptr<void>可存储任何shared_ptr<T>对象
    guard = m_pTie.lock();
    if (guard)
    {
      handleEventWithGuard(receiveTime);
    }
  }
  else
  {
    handleEventWithGuard(receiveTime);
  }
}

void Channel::handleEventWithGuard(
        TimeStamp receiveTime)
{
  m_bEventHandling = true;
  LOG_TRACE << reventsToString();
  if ((m_nRevents & POLLHUP) 
    && !(m_nRevents & POLLIN))
  {
    if (m_bLogHup)
    {
      LOG_WARN 
        << "fd = " 
        << m_nFd 
        << " Channel::handle_event() POLLHUP";
    }

    if (m_nCloseCallback)
    {
        m_nCloseCallback();
    }
  }

  if (m_nRevents & POLLNVAL)
  {
    LOG_WARN 
        << "fd = " 
        << m_nFd 
        << " Channel::handle_event() POLLNVAL";
  }

  // POLLERR表示发生错误
  if (m_nRevents & (POLLERR | POLLNVAL))
  {
    LOG_WARN 
        << "fd = " 
        << m_nFd 
        << " Channel::handle_event() ERR";

    if (m_nErrorCallback)
    {
        m_nErrorCallback();
    }
  }

  // POLLIN普通或优先级带数据可读
  // POLLPRI高优先级数据可读
  // POLLRDHUP对端关闭时
  // 一直会触发POLLIN+POLLRDHUP
  if (m_nRevents & (POLLIN | POLLPRI | POLLRDHUP))
  {
    LOG_WARN 
        << "fd = " 
        << m_nFd 
        << " Channel::handle_event() IN/PRI/RDHUP";

    if (m_nReadCallback)
    {
        m_nReadCallback(receiveTime);
    }
  }

  // POLLOUT普通数据可写
  if (m_nRevents & POLLOUT)
  {
    LOG_WARN 
        << "fd = " 
        << m_nFd 
        << " Channel::handle_event() OUT";

    if (m_nWriteCallback)
    {
        m_nWriteCallback();
    }
  }

  m_bEventHandling = false;
}

string Channel::reventsToString() const
{
  return eventsToString(m_nFd, m_nRevents);
}

string Channel::eventsToString() const
{
  return eventsToString(m_nFd, m_nEvents);
}

string Channel::eventsToString(int fd, int ev)
{
  std::ostringstream oss;
  oss << fd << ": ";
  if (ev & POLLIN)
  {
    oss << "IN ";
  }

  if (ev & POLLPRI)
  {
    oss << "PRI ";
  }

  if (ev & POLLOUT)
  {
    oss << "OUT ";
  }

  if (ev & POLLHUP)
  {
    oss << "HUP ";
  }

  if (ev & POLLRDHUP)
  {
    oss << "RDHUP ";
  }

  if (ev & POLLERR)
  {
    oss << "ERR ";
  }

  if (ev & POLLNVAL)
  {
    oss << "NVAL ";
  }

  return oss.str();
}


