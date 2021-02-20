#ifndef NLIB_POLLER_EPOLLPOLLER_H
#define NLIB_POLLER_EPOLLPOLLER_H
#include "header.h"
#include <sys/epoll.h>
#include "Poller.h"
struct epoll_event;
class EPollPoller : public Poller
{
public:
    EPollPoller(
        EventLoop* loop);
    ~EPollPoller() override;
    TimeStamp poll(
        int timeoutMs, 
        ChannelList* activeChannels) override;
    void updateChannel(
        Channel* channel) override;
    void removeChannel(
        Channel* channel) override;
private:
    static const int s_nInitEventListSize = 16;
    static const char* operationToString(
        int op);
    void fillActiveChannels(
        int numEvents,
        ChannelList* activeChannels) const;
    void update(
        int operation, 
        Channel* channel);

private:
    typedef std::vector<struct epoll_event> EventList;
    int m_nEpollFd;
    EventList m_nEvents;
};

#endif

