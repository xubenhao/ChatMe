#ifndef NLIB_COUNTDOWNLATCH_H
#define NLIB_COUNTDOWNLATCH_H
#include "header.h"
#include "Condition.h"
#include "Mutex.h"

// 一个内部支持
// 多线程间互斥／同步访问的对象类型
// 内部处理的是一个计数
class CountDownLatch  
{
public:
    explicit CountDownLatch(int count);
    void wait();
    void countDown();
    int getCount() const;
private:
    // mutable数据成员意义是
    // 以const CountDownLatch执行对该成员访问时，
    // 该成员是可修改的
    mutable MutexLock m_nMutex;
    Condition m_nCondition;
    int m_nCount;
};
#endif  

