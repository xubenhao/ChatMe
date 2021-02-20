#include "EPollPoller.h"
#include "Logging.h"
#include "Channel.h"

static_assert(
    EPOLLIN == POLLIN,        
    "epoll uses same flag values as poll");
static_assert(
    EPOLLPRI == POLLPRI,      
    "epoll uses same flag values as poll");
static_assert(
    EPOLLOUT == POLLOUT,      
    "epoll uses same flag values as poll");
static_assert(
    EPOLLRDHUP == POLLRDHUP,  
    "epoll uses same flag values as poll");
static_assert(
    EPOLLERR == POLLERR,      
    "epoll uses same flag values as poll");
static_assert(
    EPOLLHUP == POLLHUP,      
    "epoll uses same flag values as poll");

    
const int s_nNew = -1;
const int s_nAdded = 1;
const int s_nDeleted = 2;

EPollPoller::EPollPoller(
    EventLoop* loop)
  : Poller(loop),
    m_nEpollFd(::epoll_create1(EPOLL_CLOEXEC)),
    m_nEvents(s_nInitEventListSize)
{
  if (m_nEpollFd < 0)
  {
    LOG_SYSFATAL 
        << "EPollPoller::EPollPoller";
  }
}

EPollPoller::~EPollPoller()
{
  ::close(m_nEpollFd);
}

TimeStamp EPollPoller::poll(
    int timeoutMs, 
    ChannelList* activeChannels)
{
  LOG_INFO 
      << "fd total count " 
      << m_nChannels.size();
  int numEvents = ::epoll_wait(
          m_nEpollFd,
          &*m_nEvents.begin(),
          (int)(m_nEvents.size()),
          timeoutMs);
  int savedErrno = errno;
  TimeStamp now(TimeStamp::now());
  if (numEvents > 0)
  {
    fillActiveChannels(
        numEvents, 
        activeChannels);
    if ((size_t)(numEvents) == m_nEvents.size())
    {
      m_nEvents.resize(m_nEvents.size()*2);
    }
  }
  else if (numEvents == 0)
  {
    LOG_INFO 
        << "nothing happened";
  }
  else
  {

    if (savedErrno != EINTR)
    {
      errno = savedErrno;
      LOG_SYSERR 
          << "EPollPoller::poll()";
    }
  }

  return now;
}

void EPollPoller::fillActiveChannels(
    int numEvents,
    ChannelList* activeChannels) const
{
  assert((size_t)(numEvents) <= m_nEvents.size());
  for (int i = 0; i < numEvents; ++i)
  {
    Channel* channel = (Channel*)(m_nEvents[i].data.ptr);
#ifndef NDEBUG
    int fd = channel->fd();
    ChannelMap::const_iterator it = m_nChannels.find(fd);
    assert(it != m_nChannels.end());
    assert(it->second == channel);
#endif
    
    LOG_INFO 
        << " fd = " 
        << fd 
        << " event = { " 
        << channel->eventsToString(fd, m_nEvents[i].events) 
        << " }";

    channel->set_revents(m_nEvents[i].events);
    activeChannels->push_back(channel);
  }
}

void EPollPoller::updateChannel(
    Channel* channel)
{
  Poller::assertInLoopThread();
  const int index = channel->index();
  // channel的index，
  // 表明channel如何与一个EPOLL互动
  // 添加／删除／修改
  if (index == s_nNew 
    || index == s_nDeleted)
  {
    int fd = channel->fd();
    if (index == s_nNew)
    {
      assert(m_nChannels.find(fd) 
        == m_nChannels.end());
      m_nChannels[fd] = channel;
    }
    else 
    {
      assert(m_nChannels.find(fd) 
        != m_nChannels.end());
      assert(m_nChannels[fd] == channel);
    }

    channel->set_index(s_nAdded);
    update(EPOLL_CTL_ADD, channel);
  }
  else
  {
    int fd = channel->fd();
    (void)fd;
    assert(m_nChannels.find(fd) 
        != m_nChannels.end());
    assert(m_nChannels[fd] == channel);
    assert(index == s_nAdded);
    if (channel->isNoneEvent())
    {
      update(EPOLL_CTL_DEL, channel);
      channel->set_index(s_nDeleted);
    }
    else
    {
      update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EPollPoller::removeChannel(
    Channel* channel)
{
  Poller::assertInLoopThread();
  int fd = channel->fd();
  LOG_INFO 
      << "fd = " << fd;
  assert(
    m_nChannels.find(fd) 
    != m_nChannels.end());
  assert(
    m_nChannels[fd] 
    == channel);
  assert(channel->isNoneEvent());
  int index = channel->index();
  assert(index == s_nAdded 
    || index == s_nDeleted);
  size_t n = m_nChannels.erase(fd);
  assert(n == 1);

  if (index == s_nAdded)
  {
    update(EPOLL_CTL_DEL, channel);
  }

  channel->set_index(s_nNew);
}

void EPollPoller::update(
    int operation, 
    Channel* channel)
{
  struct epoll_event event;
  memset(&event, 0, sizeof event);
  event.events = channel->events();
  event.data.ptr = channel;
  int fd = channel->fd();
  LOG_INFO 
      << "epoll_ctl op = " 
      << operationToString(operation)
      << " fd = " 
      << fd 
      << " event = { " 
      << channel->eventsToString() 
      << " }";
  
  if (::epoll_ctl(
    m_nEpollFd, 
    operation, 
    fd, 
    &event) < 0)
  {
    if (operation == EPOLL_CTL_DEL)
    {
      LOG_SYSERR 
        << "epoll_ctl op =" 
        << operationToString(operation) 
        << " fd =" << fd;
    }
    else
    {
      LOG_SYSFATAL 
        << "epoll_ctl op =" 
        << operationToString(operation) 
        << " fd =" << fd;
    }
  }
}

const char* EPollPoller::operationToString(
    int op)
{
  switch (op)
  {
    case EPOLL_CTL_ADD:
      return "ADD";
    case EPOLL_CTL_DEL:
      return "DEL";
    case EPOLL_CTL_MOD:
      return "MOD";
    default:
      assert(false && "ERROR op");
      return "Unknown Operation";
  }
}

