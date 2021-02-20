#ifndef NLIB_EVENTLOOP_H
#define NLIB_EVENTLOOP_H
#include "header.h"
#include "CallBacks.h"
#include "TimerId.h"
#include "Mutex.h"

class Channel;
class Poller;
class TimerQueue;

class EventLoop
{
public: 
    typedef std::function<void()> Functor;  
    EventLoop();
    ~EventLoop(); 
    void loop();
    void quit();  
    TimeStamp pollReturnTime() const 
    { 
        return m_nPollReturnTime; 
    }
    
    int64_t iteration() const 
    { 
        return m_nIteration; 
    }
        
    void runInLoop(
        Functor cb);
    void queueInLoop(
        Functor cb);
    size_t queueSize() const; 
    TimerId runAt(
        TimeStamp time, 
        TimerCallback cb);  
    TimerId runAfter(
        double delay, 
        TimerCallback cb);
    TimerId runEvery(
        double interval, 
        TimerCallback cb);  
    void cancel(
        TimerId timerId);  
    
    void wakeup();  
    
    void updateChannel(
        Channel* channel);
    void removeChannel(
        Channel* channel);  
    bool hasChannel(
        Channel* channel); 
    
    void assertInLoopThread()  
    {
        if (!isInLoopThread())
        {
            abortNotInLoopThread();
        }  
    }
  
    bool isInLoopThread() const  
    {  
        return m_nThreadId == tid();  
    }
  
    bool eventHandling() const  
    {  
        return m_bEventHandling;  
    }
    
    // boost::any可用于保存任意类型
    // boost::any::type()用于返回对象实际类型信息
    // boost::any_cast<xxx>()
    // 用于将一个boost::any对象强制转换为指定类型
    void setContext(const boost::any& context)  
    {  
        m_nContext = context;  
    }
    
    const boost::any& getContext() const  
    {  
        return m_nContext;  
    }

    boost::any* getMutableContext()  
    {  
        return &m_nContext;  
    }
  
    static EventLoop* getEventLoopOfCurrentThread();     
private:  
    void abortNotInLoopThread();  
    void handleRead();   
    void doPendingFunctors(); 
    void printActiveChannels() const;   
    typedef std::vector<Channel*> ChannelList;
    
private:
    bool m_bLooping;  
    std::atomic<bool> m_nQuit; 
    bool m_bEventHandling;  
    bool m_bCallingPendingFunctors;
    int64_t m_nIteration;  
    
    const pid_t m_nThreadId;  
    TimeStamp m_nPollReturnTime;
    std::unique_ptr<Poller> m_pPoller;  
    std::unique_ptr<TimerQueue> m_pTimerQueue;  
    int m_nWakeupFd;
    std::unique_ptr<Channel> m_pWakeupChannel;  
    
    boost::any m_nContext; 
    ChannelList m_nActiveChannels;
    Channel* m_pCurrentActiveChannel;
    std::vector<Functor> m_vecPendingFunctors;

    mutable MutexLock m_nMutex; 
};    


#endif

