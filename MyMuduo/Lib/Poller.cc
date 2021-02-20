#include "Poller.h"
#include "Channel.h"
#include "PollPoller.h"
#include "EPollPoller.h"
#include "Logging.h"

Poller::Poller(EventLoop* loop)
    : m_pOwnerLoop(loop)
{
}

Poller::~Poller() = default;
bool Poller::hasChannel(Channel* channel) const
{
    assertInLoopThread();
    ChannelMap::const_iterator it = 
        m_nChannels.find(channel->fd());
    return it != m_nChannels.end() 
        && it->second == channel;
}

Poller* Poller::newDefaultPoller(
        EventLoop* loop)
{
  /*if (::getenv("MUDUO_USE_POLL"))
  {
      LOG_INFO
          << "PollPoller";
      return new PollPoller(loop);

  }
  else*/
  {
    LOG_INFO
        << "EPollPoller";
    return new EPollPoller(loop);
  }
}


