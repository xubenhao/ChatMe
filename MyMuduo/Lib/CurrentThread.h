#ifndef NLIB_CURRENTTHREAD_H
#define NLIB_CURRENTTHREAD_H
#include "header.h"
#include "TimeStamp.h"
// __thread修饰的全局对象
// 每个线程有各自独立的一份，
// 天然就是线程特定的数据
extern __thread int t_cachedTid;
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;
extern __thread const char* t_threadName;

pid_t gettid();      
void cacheTid();
bool isMainThread();
void sleepUsec(int64_t usec);  
string stackTrace(bool demangle);    

inline int tid()
{
    cacheTid();
    return t_cachedTid;
}
      
inline const char* tidString() 
{
    return t_tidString;
}
    
inline int tidStringLength() 
{
    return t_tidStringLength;
}
    
inline const char* name()
{
    return t_threadName;
}
    
#endif  

