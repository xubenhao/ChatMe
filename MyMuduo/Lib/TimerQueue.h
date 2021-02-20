#ifndef LIB_TIMERQUEUE_H
#define LIB_TIMERQUEUE_H

#include "Mutex.h"
#include "TimeStamp.h"
#include "CallBacks.h"
#include "Channel.h"
    
class EventLoop;    
class Timer;    
class TimerId;    
class TimerQueue     
{
public: 
    explicit TimerQueue(
            EventLoop* loop);  
    ~TimerQueue();
    TimerId addTimer(
        TimerCallback cb,            
        TimeStamp when,             
        double interval);
    void cancel(
            TimerId timerId);
private:
    typedef std::pair<TimeStamp, Timer*> 
        Entry;
    typedef std::set<Entry> TimerList;
    typedef std::pair<Timer*, int64_t> 
        ActiveTimer;
    typedef std::set<ActiveTimer> 
        ActiveTimerSet;
    

    void addTimerInLoop(
            Timer* timer);  
    void cancelInLoop(
            TimerId timerId);
    void handleRead();
    std::vector<Entry> getExpired(
            TimeStamp now);
    void reset(
            const std::vector<Entry>& expired, 
            TimeStamp now);
    bool insert(
            Timer* timer);
    

private:
    EventLoop* m_pLoop;
    const int m_nTimerFd;
    Channel m_nTimerFdChannel;
    
    // 二者总是保持一致
    TimerList m_nTimers;
    ActiveTimerSet m_nActiveTimers;

    bool m_bCallingExpiredTimers;
    ActiveTimerSet m_nCancelingTimers;
};

#endif


