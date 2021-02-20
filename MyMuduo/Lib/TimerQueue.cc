#include "TimerQueue.h"
#include "Logging.h"   
#include "Timer.h"
#include "TimerId.h"

int createTimerfd()           
{   
    int timerfd = ::timerfd_create(    
            CLOCK_MONOTONIC,    
            // 从系统启动开始计时
            TFD_NONBLOCK | TFD_CLOEXEC);    
    if (timerfd < 0)          
    {        
        LOG_SYSFATAL 
            << "Failed in timerfd_create";          
    }
                 
    return timerfd;            
}
            
// 参数为截止时间 
struct timespec howMuchTimeFromNow(
        TimeStamp when)            
{          
    int64_t microseconds = 
        when.microSecondsSinceEpoch()
        - TimeStamp::now().microSecondsSinceEpoch();          
    if (microseconds < 100)          
    {        
        microseconds = 100;          
    }
                  
    struct timespec ts;          
    ts.tv_sec = (time_t)(      
            microseconds / TimeStamp::s_nMicroSecondsPerSecond);          
    ts.tv_nsec = (long)(      
            (microseconds % TimeStamp::s_nMicroSecondsPerSecond) * 1000);      
    return ts;            
}
    
void readTimerfd(
        // 发生超时的定时器描述符
        int timerfd, 
        // 当前时间戳
        TimeStamp now)            
{          
    uint64_t howmany;      
    ssize_t n = read(
            timerfd, 
            &howmany, 
            sizeof(howmany));   
    if (n != sizeof howmany)          
    {        
        LOG_ERROR 
            << "TimerQueue::handleRead() reads " 
            << n << " bytes instead of 8";          
    }            
}
            
void resetTimerfd(
    int timerfd, 
    TimeStamp expiration)         
{    
    struct itimerspec newValue;    
    struct itimerspec oldValue;    
    memset(
            &newValue, 
            0, 
            sizeof newValue);    
    memset(
            &oldValue, 
            0, 
            sizeof oldValue);   
    // 存放的是时间差 
    newValue.it_value = 
        howMuchTimeFromNow(expiration);
    // 通过timerfd描述符设置一个在指定时间后会超时的定时器
    int ret = ::timerfd_settime(
            timerfd, 
            0, 
            &newValue, 
            &oldValue);    
    if (ret)    
    {    
        LOG_SYSERR 
            << "timerfd_settime()";    
    }    
}

TimerQueue::TimerQueue(
        EventLoop* loop)
    : m_pLoop(loop),
    m_nTimerFd(createTimerfd()),
    // 事件循环＆描述符
    m_nTimerFdChannel(
            loop, 
            m_nTimerFd),
    m_nTimers(),
    m_bCallingExpiredTimers(false)
{
    
    // m_nTimerFd在定时器超时发生时，该描述符变为可读
    m_nTimerFdChannel.setReadCallback(
            std::bind(
                &TimerQueue::handleRead, 
                this));
        
    m_nTimerFdChannel.enableReading();
}

TimerQueue::~TimerQueue()
{
    m_nTimerFdChannel.disableAll();
    m_nTimerFdChannel.remove();
    ::close(m_nTimerFd);
    
    // Entry是时间戳对象
    // 和Timer对象指针的一个组合 
    for (const Entry& timer : m_nTimers)
    {
        delete timer.second;
    }
}

TimerId TimerQueue::addTimer(
        TimerCallback cb,                 
        TimeStamp when,                     
        double interval)
{
    // 记录了定时器的各种属性
    Timer* timer = new Timer(
            std::move(cb), 
            when, 
            interval);
    // 借助事件循环执行特定函数调用
    m_pLoop->runInLoop(
        std::bind(
            &TimerQueue::addTimerInLoop, 
            this, 
            timer));
    // 定时器对象，对象序列号的复合结构
    return TimerId(
            timer, 
            timer->sequence());
}

void TimerQueue::cancel(
        TimerId timerId)
{
    m_pLoop->runInLoop(
        std::bind(
            &TimerQueue::cancelInLoop, 
            this, 
            timerId));
}

void TimerQueue::addTimerInLoop(
        Timer* timer)
{
    m_pLoop->assertInLoopThread();
    // 将参数放入所维护的集合 
    bool earliestChanged = insert(timer);
    // 若参数将是最早触发超时的那个
    if (earliestChanged)
    {
        resetTimerfd(
                m_nTimerFd, 
                timer->expiration());
    }
}

void TimerQueue::cancelInLoop(
        TimerId timerId)
{
    m_pLoop->assertInLoopThread();
    assert(m_nTimers.size() == m_nActiveTimers.size());
    ActiveTimer timer(
            timerId.m_pTimer, 
            timerId.m_nSequence);
    ActiveTimerSet::iterator it = m_nActiveTimers.find(timer);
    if (it != m_nActiveTimers.end())
    {
        size_t n = m_nTimers.erase(
                Entry(it->first->expiration(), it->first));
        assert(n == 1); (void)n;
        delete it->first;
        m_nActiveTimers.erase(it);
    }
    else if (m_bCallingExpiredTimers)
    {
        m_nCancelingTimers.insert(timer);
    }
}

void TimerQueue::handleRead()
{
    m_pLoop->assertInLoopThread();
    TimeStamp now(TimeStamp::now());
    readTimerfd(
            m_nTimerFd, 
            now);

    std::vector<Entry> expired = getExpired(now);
    m_bCallingExpiredTimers = true;
    m_nCancelingTimers.clear();
    
    // 定时器超时了，
    // 寻找在此刻超时的各个定时器所关联的回调函数执行回调
    for (const Entry& it : expired)
    {
        it.second->run();
    }

    m_bCallingExpiredTimers = false;
    
    // 对超时的定时器依据触发方式是间隔触发还是触发一次
    // 对其进行更新
    reset(
            expired, 
            now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(
        TimeStamp now)
{
    assert(m_nTimers.size() == m_nActiveTimers.size());
    std::vector<Entry> expired;
    Entry sentry(
            now, 
            (Timer*)(UINTPTR_MAX));
    TimerList::iterator end = m_nTimers.lower_bound(sentry);
    assert(end == m_nTimers.end() || now < end->first);
    std::copy(
            m_nTimers.begin(), 
            end, 
            back_inserter(expired));
    m_nTimers.erase(m_nTimers.begin(), end);
    for (const Entry& it : expired)
    {
        ActiveTimer timer(
                it.second, 
                it.second->sequence());
        size_t n = m_nActiveTimers.erase(timer);
        assert(n == 1); (void)n;
    }

    assert(m_nTimers.size() == m_nActiveTimers.size());
    return expired;
}

void TimerQueue::reset(
    const std::vector<Entry>& expired, 
    TimeStamp now)
{
    TimeStamp nextExpire;
    for (const Entry& it : expired)
    {
        ActiveTimer timer(
                it.second, 
                it.second->sequence());
        if (it.second->repeat()
            && m_nCancelingTimers.find(timer) 
            == m_nCancelingTimers.end())
        {
            it.second->restart(now);
            insert(it.second);
        }
        else
        {
            delete it.second;
        }
    }
    
    if (!m_nTimers.empty())
    {
        nextExpire 
            = m_nTimers.begin()->second->expiration();
    }

    if (nextExpire.valid())
    {
        resetTimerfd(m_nTimerFd, nextExpire);
    }
}

// 会把传入参数集成到
// m_nTimers和m_nActiveTimers集合中
// 返回值表示参数指向的Timer对象
// 在维护的此对象集合中是否属于
// 最早被触发的那个
bool TimerQueue::insert(
        Timer* timer)
{
    m_pLoop->assertInLoopThread();
    assert(
            m_nTimers.size() == m_nActiveTimers.size());
    
    bool earliestChanged = false;
    // 传入定时器的超时触发事件
    TimeStamp when = timer->expiration();
    TimerList::iterator it = m_nTimers.begin();
    
    if (it == m_nTimers.end() 
            || when < it->first)
    {
        earliestChanged = true;
    }
  
    {
        std::pair<TimerList::iterator, bool> result = 
            // 一个触发事件和定时器
            // 构成的复合结构
            //
            m_nTimers.insert(
                    Entry(when, timer));
        assert(result.second); 
        (void)result;
    }
  

    {
        std::pair<ActiveTimerSet::iterator, bool> result = 
            m_nActiveTimers.insert(
                    ActiveTimer(timer, 
                        timer->sequence()));
    }

    
    assert(m_nTimers.size() == m_nActiveTimers.size());
    return earliestChanged;
}

