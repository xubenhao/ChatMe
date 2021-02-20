#ifndef NLIB_EVENTLOOPTHREADPOOL_H
#define NLIB_EVENTLOOPTHREADPOOL_H
#include "header.h"

// 事件循环线程池对象
// 可由可访问者用以实现
// １．开启多个并行线程
// ２．获得开启的每个线程的属性／状态
// ３．与每个开启的线程进行交互
//
//
// 这里假定了
// 事件循环线程池对象的访问者
// 存在于一个线程中．
// 所以，
// 线程池对象成员访问中没有进行多线程下访问共享对象的互斥／同步处理
class EventLoop;
class EventLoopThread;
class EventLoopThreadPool 
{
public:
    typedef std::function<void(EventLoop*)> ThreadInitCallback;
    EventLoopThreadPool(
        EventLoop* baseLoop, 
        const string& nameArg);
    ~EventLoopThreadPool();
    void setThreadNum(
            int numThreads) 
    { 
        m_nNumThreads = numThreads; 
    }

    void start(
        const ThreadInitCallback& cb = ThreadInitCallback());
    
    EventLoop* getNextLoop();
    EventLoop* getLoopForHash(
        size_t hashCode);
    std::vector<EventLoop*> getAllLoops();
    bool started() const
    {
        return m_bStarted;
    }

    const string& name() const
    {
        return m_strName;
    }

private:
    EventLoop* m_pBaseLoop;
    string m_strName;
    bool m_bStarted;
    int m_nNumThreads;
    int m_nNext;
    std::vector<std::unique_ptr<EventLoopThread>> m_vecThreads;
    std::vector<EventLoop*> m_vecLoops;
};


#endif

