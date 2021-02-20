#ifndef NLIB_POLLER_H
#define NLIB_POLLER_H
#include "header.h"
#include "TimeStamp.h"
#include "EventLoop.h"

class Channel;
class Poller 
{
public:
    typedef std::vector<Channel*> ChannelList;
    Poller(
        EventLoop* loop);
    virtual ~Poller();
    virtual TimeStamp poll(
        int timeoutMs, 
        ChannelList* activeChannels) = 0;
    virtual void updateChannel(
        Channel* channel) = 0;
    virtual void removeChannel(
        Channel* channel) = 0;
    virtual bool hasChannel(
        Channel* channel) const;
    void assertInLoopThread() const
    {
        m_pOwnerLoop->assertInLoopThread();
    }

    static Poller* newDefaultPoller(
        EventLoop* loop);
     
protected:
    typedef std::map<int, Channel*> ChannelMap;
    ChannelMap m_nChannels;
private:
    EventLoop* m_pOwnerLoop;
};

#endif

