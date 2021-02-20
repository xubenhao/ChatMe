#ifndef LIB_TIMER_H
#define LIB_TIMER_H

#include "Atomic.h"
#include "TimeStamp.h"
#include "CallBacks.h"

// 将定时器回调函数作为类的数据成员的好处：
// 类对象构成了定时器回调及其回调语境
// 
class Timer 
{
public:
    Timer(
        // 定时器回调函数
        TimerCallback cb,
        // 何时触发时间戳
        TimeStamp when, 
        // 是否间隔性触发
        double interval)
        : m_nCallBack(
                std::move(cb)),
        
        m_nExpiration(when),
        m_nInterval(interval),
        m_bRepeat(interval > 0.0),
        m_nSequence(
                s_nNumCreated.incrementAndGet())
    { 

    }
    
    // 执行定时器回调函数
    void run() const
    {
        m_nCallBack();
    }

    TimeStamp expiration() const  
    { 
        return m_nExpiration; 
    }
    
    bool repeat() const 
    { 
        return m_bRepeat; 
    }
    
    int64_t sequence() const 
    { 
        return m_nSequence; 
    }
    
    void restart(TimeStamp now);
    static int64_t numCreated() 
    { 
        return 
            s_nNumCreated.get(); 
    }

private:
    const TimerCallback 
        m_nCallBack;
    TimeStamp m_nExpiration;
    const double m_nInterval;
    const bool m_bRepeat;
    const int64_t m_nSequence;
    static AtomicInt64 s_nNumCreated;
};
  

#endif  

