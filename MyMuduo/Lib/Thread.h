#ifndef NLIB_THREAD_H
#define NLIB_THREAD_H
#include "header.h"
#include "Atomic.h"
#include "CountDownLatch.h"

// 提供线程启动接口
// 包含线程回调指定，线程信息保持

// 通过一个Thread类成员函数内部调线程api
// 为线程api提供了语境
// 成员函数内部通过this指向对象的数据成员
// 可以得到调用发生时需要的语境信息
// 并可据此作对应的处理
//
class Thread  
{
public:
    typedef std::function<void ()> ThreadFunc;
    explicit Thread(
        ThreadFunc, 
        const string& name = string());
    
    ~Thread();
    void start();
    int join();
    bool started() const 
    { 
        return m_bStarted; 
    }
    
    pid_t tid() const 
    { 
        return m_nTid; 
    }

    const string& name() const 
    { 
        return m_strName; 
    }

    static int numCreated() 
    { 
        return s_nNumCreated.get(); 
    }

private:
    void setDefaultName();
    
private:
    bool m_bStarted;
    bool m_bJoined;
    pthread_t m_nPthreadId;
    pid_t m_nTid;
    
    ThreadFunc m_nFunc;
    string m_strName;
    CountDownLatch m_nLatch;
    static AtomicInt32 s_nNumCreated;
};

#endif

