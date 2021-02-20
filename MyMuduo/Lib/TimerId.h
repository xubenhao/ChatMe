#ifndef LIB_TIMERID_H
#define LIB_TIMERID_H
#include "CallBacks.h"    
class Timer;    

// 标识一个定时器
class TimerId    
{
public:  
    TimerId()
        : m_pTimer(NULL),
        m_nSequence(0)  
    {  
    }

    TimerId(Timer* timer, int64_t seq)
        : m_pTimer(timer),
        m_nSequence(seq)  
    {  
    }
    
    friend class TimerQueue;
private:  
    Timer* m_pTimer;  
    int64_t m_nSequence;    
};    


#endif

