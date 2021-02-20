#ifndef NLIB_MUTEX_H
#define NLIB_MUTEX_H
#include "header.h"
#include "CurrentThread.h"
   
// 互斥锁，同时包含拥有此锁的线程id
class MutexLock 
{
    class UnassignGuard 
    {
    public:
        explicit UnassignGuard(
            MutexLock& owner)
          : m_nOwner(owner)
        {
          m_nOwner.unassignHolder();
        }
        
        ~UnassignGuard()
        {
          m_nOwner.assignHolder();
        }
       
    private:
        MutexLock& m_nOwner;
    }; 

public:
    MutexLock()
        : m_nHolder(0)
    {
        pthread_mutex_init(&m_nMutex, NULL);
    }
    
    ~MutexLock()
    {
        assert(m_nHolder == 0);
        pthread_mutex_destroy(&m_nMutex);
    }
    
    bool isLockedByThisThread() const
    {
        // m_nHolder记录拥有互斥锁的线程id
        return m_nHolder == tid();
    }
    
    void assertLocked() const
    {
        assert(isLockedByThisThread());
    }
    
    void lock()
    {
        pthread_mutex_lock(&m_nMutex);
        assignHolder();
    }
    
    void unlock()
    {
        unassignHolder();
        pthread_mutex_unlock(&m_nMutex);
    }
    
    pthread_mutex_t* getPthreadMutex() 
    {
        return &m_nMutex;
    }
    
private:
    void unassignHolder()
    {
        m_nHolder = 0;
    }
    
    void assignHolder()
    {
        // tid得到执行此调用线程的id
        m_nHolder = tid();
    }
    
private:
    pthread_mutex_t m_nMutex;
    pid_t m_nHolder;
    friend class Condition;
};

class MutexLockGuard 
{
public:
    explicit MutexLockGuard(MutexLock& mutex)
        : m_nMutex(mutex)
    {
        m_nMutex.lock();
    }
    
    ~MutexLockGuard()
    {
        m_nMutex.unlock();
    }
    
private:
    MutexLock& m_nMutex;
};

#endif  

