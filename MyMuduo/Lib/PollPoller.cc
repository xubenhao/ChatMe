#include "PollPoller.h"
#include "Logging.h"
#include "Channel.h"

PollPoller::PollPoller(EventLoop* loop)
  : Poller(loop)
{
}

PollPoller::~PollPoller() = default;

TimeStamp PollPoller::poll(
    int timeoutMs, 
    ChannelList* activeChannels)
{
  int numEvents = ::poll(
    &*m_nPollFds.begin(), 
    m_nPollFds.size(), 
    timeoutMs);
  int savedErrno = errno;
  TimeStamp now(TimeStamp::now());
  if (numEvents > 0)
  {
    LOG_TRACE 
        << numEvents 
        << " events happened";
    fillActiveChannels(numEvents, activeChannels);
  }
  else if (numEvents == 0)
  {
    LOG_TRACE 
        << " nothing happened";
  }
  else
  {
    if (savedErrno != EINTR)
    {
      errno = savedErrno;
      LOG_SYSERR 
          << "PollPoller::poll()";
    }
  }

  return now;
}

void PollPoller::fillActiveChannels(
    int numEvents,
    ChannelList* activeChannels) const
{
  for (PollFdList::const_iterator pfd = m_nPollFds.begin();
      pfd != m_nPollFds.end() && numEvents > 0; 
      ++pfd)
  {
    if (pfd->revents > 0)
    {
      --numEvents;
      ChannelMap::const_iterator ch = 
          m_nChannels.find(pfd->fd);
      assert(ch != m_nChannels.end());
      Channel* channel = ch->second;
      assert(channel->fd() == pfd->fd);
      channel->set_revents(pfd->revents);
      activeChannels->push_back(channel);
    }
  }
}

void PollPoller::updateChannel(Channel* channel)
{
  Poller::assertInLoopThread();
  LOG_TRACE 
      << "fd = " 
      << channel->fd() 
      << " events = " 
      << channel->events();
  if (channel->index() < 0)
  {
    assert(m_nChannels.find(channel->fd()) == 
            m_nChannels.end());
    struct pollfd pfd;
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    m_nPollFds.push_back(pfd);
    int idx = static_cast<int>(m_nPollFds.size())-1;
    channel->set_index(idx);
    m_nChannels[pfd.fd] = channel;
  }
  else
  {
    assert(m_nChannels.find(channel->fd()) != m_nChannels.end());
    assert(m_nChannels[channel->fd()] == channel);
    int idx = channel->index();
    assert(0 <= idx 
            && idx < static_cast<int>(m_nPollFds.size()));
    struct pollfd& pfd = m_nPollFds[idx];
    assert(pfd.fd == channel->fd() 
            || pfd.fd == -channel->fd()-1);
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    if (channel->isNoneEvent())
    {
      pfd.fd = -channel->fd()-1;
    }
  }
}

void PollPoller::removeChannel(Channel* channel)
{
  Poller::assertInLoopThread();
  LOG_TRACE 
      << "fd = " 
      << channel->fd();
  assert(m_nChannels.find(channel->fd()) != m_nChannels.end());
  assert(m_nChannels[channel->fd()] == channel);
  assert(channel->isNoneEvent());
  int idx = channel->index();
  assert(0 <= idx 
    && idx < static_cast<int>(m_nPollFds.size()));
  const struct pollfd& pfd = m_nPollFds[idx]; (void)pfd;
  assert(pfd.fd == -channel->fd()-1 
    && pfd.events == channel->events());
  size_t n = m_nChannels.erase(channel->fd());
  assert(n == 1); (void)n;
  if ((size_t)(idx) == m_nPollFds.size()-1)
  {
    m_nPollFds.pop_back();
  }
  else
  {
    int channelAtEnd = m_nPollFds.back().fd;
    iter_swap(m_nPollFds.begin()+idx, m_nPollFds.end()-1);
    if (channelAtEnd < 0)
    {
      channelAtEnd = -channelAtEnd-1;
    }
    
    m_nChannels[channelAtEnd]->set_index(idx);
    m_nPollFds.pop_back();
  }
}

