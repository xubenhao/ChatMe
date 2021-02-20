#ifndef NLIB_EVENTLOOPTHREAD_H
#define NLIB_EVENTLOOPTHREAD_H

#include "header.h"
#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"
    
// 从一个类型得到新类型
// １．继承
// ２．组合
class EventLoop;
class EventLoopThread 
{
public:
    typedef std::function<void(EventLoop*)> 
        ThreadInitCallback;
    EventLoopThread(
        const ThreadInitCallback& cb = ThreadInitCallback(),
        const string& name = string());
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();
    
private:
    bool m_bExiting;
    
    Thread m_nThread;
    
    MutexLock m_nMutex;
    Condition m_nCond;
    EventLoop* m_pLoop;

    ThreadInitCallback m_nCallBack;
};
    


#endif

