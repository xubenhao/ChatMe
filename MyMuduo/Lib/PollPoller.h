#ifndef NLIB_POLLER_POLLPOLLER_H
#define NLIB_POLLER_POLLPOLLER_H

#include "header.h"
#include "Poller.h"

struct pollfd;
class PollPoller : public Poller
{
public:
    PollPoller(
        EventLoop* loop);
    ~PollPoller() override;
    TimeStamp poll(
        int timeoutMs, 
        ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;
private:
    void fillActiveChannels(
        int numEvents,
        ChannelList* activeChannels) const;
    typedef std::vector<struct pollfd> PollFdList;
    PollFdList m_nPollFds;
};

#endif

