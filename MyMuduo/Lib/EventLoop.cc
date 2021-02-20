#include "EventLoop.h"
#include "Logging.h"
#include "Channel.h"
#include "Poller.h"
#include "SocketOps.h"
#include "TimerQueue.h"

__thread EventLoop* t_loopInThisThread = 0; 
const int s_nPollTimeMs = 10000;    
int createEventfd()    
{
    int evtfd = ::eventfd(
        0,
        EFD_NONBLOCK | EFD_CLOEXEC);  
    if (evtfd < 0)  
    {
        LOG_SYSERR 
            << "Failed in eventfd";
        abort();  
    }
        
    return evtfd;    
}
    
class IgnoreSigPipe    
{
public:  
    IgnoreSigPipe()  
    {
        ::signal(SIGPIPE, SIG_IGN);  
    }    
};

EventLoop* EventLoop::getEventLoopOfCurrentThread()   
{
    return t_loopInThisThread;    
}
    

EventLoop::EventLoop()
    : m_bLooping(false),
    m_nQuit(false),
    m_bEventHandling(false),
    m_bCallingPendingFunctors(false),
    m_nIteration(0),
    m_nThreadId(tid()),
    m_pPoller(
        Poller::newDefaultPoller(this)),
    m_pTimerQueue(new TimerQueue(this)),
    m_nWakeupFd(createEventfd()), 
    m_pWakeupChannel(new Channel(this, m_nWakeupFd)),
    m_pCurrentActiveChannel(NULL)
{
    LOG_DEBUG 
        << "EventLoop created " 
        << this 
        << " in thread " 
        << m_nThreadId;
    if (t_loopInThisThread)
    {
        LOG_FATAL 
            << 
            "Another EventLoop " 
            << t_loopInThisThread
            << " exists in this thread " 
            << m_nThreadId;
    }

    // t_loopInThisThread每个线程仅有一个
    // 指向线程首次创建的EventLoop对象
    // 正常下，一个线程也只会创建唯一一个EventLoop对象
    // 每个EventLoop对象记录有创建它的线程的线程id
    if(t_loopInThisThread == NULL)
    {
        t_loopInThisThread = this;
    }

    m_pWakeupChannel->setReadCallback(
            std::bind(&EventLoop::handleRead, this));
    m_pWakeupChannel->enableReading();
}

EventLoop::~EventLoop()
{
    LOG_DEBUG 
        << "EventLoop " 
        << this 
        << " of thread " 
        << m_nThreadId  
        << " destructs in thread " 
        << tid();

    // Channel是对象包含的成员构成其所包含的描述符的语境
    // 描述符在epoll里包含指向作为其语境的Channel对象指针
    //
    // 允许Channel用disableAll/remove两个步骤与epoll交互
    // 而非disableAll里自动执行remove
    // 可以实现
    // 用单独的disableAll表达
    // 一个epoll里原来存在的描述符被取消了
    // 但其语境信息仍然存储在epoll的描述符语境集合里
    // 可以理解为死了但还未回收．
    // 用单独的remove表达
    // 确保Channel对应的描述符先是在epoll里被取消
    // 且Channel从epoll维护的语句集合里也被删除
    m_pWakeupChannel->disableAll();
    m_pWakeupChannel->remove();
    ::close(m_nWakeupFd);
    t_loopInThisThread = NULL;
}

// 多线程环境下，对类的成员函数及数据成员
// 要时时考虑，该成员函数／数据成员是否存在多线程下
// 基于共享类对象，被多个线程并行访问的情形．
// 如果存在此情形，
// 目前做法是否可保证多线程基于共享对象访问时的互斥／同步．
//
// 有些时候，理论上存在上述情形
// 但通过人为控制，实际项目中不存在，
// 为了编码简单，
// 可以当做不存在多线程情形处理，简化编程
void EventLoop::loop()
{
    assertInLoopThread();
    m_bLooping = true;
    m_nQuit = false;
    LOG_INFO 
        << "EventLoop " 
        << this 
        << " start looping";
    int _nCount = 0;
    while (!m_nQuit)
    {
        LOG_INFO 
            << "loop"
            << ++_nCount;
        m_nActiveChannels.clear();
        m_nPollReturnTime = m_pPoller->poll(
                s_nPollTimeMs, 
                &m_nActiveChannels);
        ++m_nIteration;
        if (Logger::logLevel() <= Logger::TRACE)
        {
            printActiveChannels();
        }

        m_bEventHandling = true;
        int _nN = 0;
        for (Channel* channel : m_nActiveChannels)
        {
            m_pCurrentActiveChannel = channel;
            // 每个通道都执行自己的事件处理
            m_pCurrentActiveChannel->handleEvent(m_nPollReturnTime);
        }

        m_pCurrentActiveChannel = NULL;
        m_bEventHandling = false;
        doPendingFunctors();
    }
    
    LOG_INFO 
        << "EventLoop " 
        << this 
        << " stop looping";
    m_bLooping = false;
}

void EventLoop::quit()
{
    m_nQuit = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(
        Functor cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(
        Functor cb)
{
    {
        // 存在多个线程通过一个共享的EventLoop对象
        // 执行此调用修改m_vecPendingFunctors
        // 及执行其他调用访问m_vecPendingFunctors对象问题
        // 如何保证互斥：
        // 所有对m_vecPendingFunctors访问地方
        // 加锁保护－－锁也许是多线程共享对象
        MutexLockGuard lock(m_nMutex);
        m_vecPendingFunctors.push_back(std::move(cb));
    }

    if (!isInLoopThread() 
            || m_bCallingPendingFunctors)
    {
        wakeup();
    }
}

size_t EventLoop::queueSize() const
{
    MutexLockGuard lock(m_nMutex);
    return m_vecPendingFunctors.size();
}

// std::function对象可向普通的类对象一样
// 执行拷贝构造／拷贝赋值
// 若std::function通过std::bind得到
// 且std::bind绑定了实参，
// 实参会随着std::function的拷贝而被拷贝
TimerId EventLoop::runAt(
        TimeStamp time, 
        TimerCallback cb)
{
    return m_pTimerQueue->addTimer(
            std::move(cb), 
            time, 
            0.0);
}

TimerId EventLoop::runAfter(
        double delay, 
        TimerCallback cb)
{
    TimeStamp time(
            addTime(TimeStamp::now(), delay));
    return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(
        double interval, 
        TimerCallback cb)
{
    TimeStamp time(
            addTime(TimeStamp::now(), 
            interval));
    return m_pTimerQueue->addTimer(
            std::move(cb), 
            time, 
            interval);
}

void EventLoop::cancel(TimerId timerId)
{
    return m_pTimerQueue->cancel(timerId);
}

void EventLoop::updateChannel(
        Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    m_pPoller->updateChannel(channel);
}

void EventLoop::removeChannel(
        Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if (m_bEventHandling)
    {
        // 期待：
        // １．要删除的Channel在处理中，
        // 这属于描述符事件处理中销毁自己
        // ［在epoll中取消自己，
        // 且在epoll维护的描述符语境集合中删除自己的语境对象
        // 即channel］
        // ２．要删除的Channel对应的描述符
        // 在此刻没有事件发生
        assert(m_pCurrentActiveChannel == channel 
            || std::find(
                m_nActiveChannels.begin(), 
                m_nActiveChannels.end(), 
                channel) 
            == m_nActiveChannels.end());
    }
    
    m_pPoller->removeChannel(channel);
}

bool EventLoop::hasChannel(
        Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    // epoll的描述符语境集合中是否有自己
    return m_pPoller->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL 
        << "EventLoop::abortNotInLoopThread - EventLoop " 
        << this
        << " was created in threadId_ = " 
        << m_nThreadId  
        << ", current thread id = " 
        <<  tid();
}

// 对m_nWakeupFd写和读的内容本身不重要
// 重要的是因为写，
// 而使得写之后的对epoll的wait可以立即返回
//
//
// 对epoll的交互，
// 全部在loop中进行
// 对epoll的交互始终在单线程环境
// 上述通过EventLoop类的接口设计与实现来保证的
void EventLoop::wakeup()
{
    LOG_INFO
        << "wakeup";
    uint64_t one = 1;
    ssize_t n = write(m_nWakeupFd, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR 
            << "EventLoop::wakeup() writes " 
            << n 
            << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(m_nWakeupFd, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR 
            << "EventLoop::handleRead() reads " 
            << n 
            << " bytes instead of 8";
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    m_bCallingPendingFunctors = true;
    {
        // 既要处理
        // 设计下实际存在的多线程通过共享的一个EventLoop
        // 并行操作m_vecPendingFunctors的互斥问题
        // ［目前使用下，不存在同步问题］
        // 又要
        // 保证锁定的范围尽可能的小，可提高并发性．
        //
        // 这里通过执行对象交换来实现锁定范围的尽可能小
        // 同时，不会遗漏各个线程的每个回调请求
        MutexLockGuard lock(m_nMutex);
        functors.swap(m_vecPendingFunctors);
    }
    
    for (const Functor& functor : functors)
    {
        functor();
    }

    m_bCallingPendingFunctors = false;
}

void EventLoop::printActiveChannels() const
{
    for (const Channel* channel : m_nActiveChannels)
    {
        LOG_INFO 
            << "{" 
            << channel->reventsToString() 
            << "} ";
    }
}

