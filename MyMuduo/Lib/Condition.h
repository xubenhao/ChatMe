#ifndef NLIB_CONDITION_H
#define NLIB_CONDITION_H

#include "Mutex.h"
class Condition  
{ 
public:
    explicit Condition(
        MutexLock& mutex)
        : m_nMutex(mutex)
    {
        pthread_cond_init(&m_nCond, NULL);
    }
      
    ~Condition()
    {
        pthread_cond_destroy(&m_nCond);
    }

    void wait()
    {
        // 在对象存在期间，互斥锁的拥有线程id为０
        // 在对象析构时，设置互斥锁的拥有线程id为执行对象析构的线程id
        MutexLock::UnassignGuard ug(m_nMutex);
        pthread_cond_wait(
            &m_nCond, 
            m_nMutex.getPthreadMutex());
    }
    
    bool waitForSeconds(
        double seconds);
    void notify()
    {
        pthread_cond_signal(&m_nCond);
    }
    
    void notifyAll()
    {
        pthread_cond_broadcast(&m_nCond);
    }

private:
    MutexLock& m_nMutex;
    pthread_cond_t m_nCond;
};

#endif

