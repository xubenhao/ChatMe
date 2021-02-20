#include "Condition.h"
bool Condition::waitForSeconds(
    double seconds)
{
  struct timespec abstime;
  clock_gettime(
    CLOCK_REALTIME, 
    &abstime);
  const int64_t s_nNanoSecondsPerSecond = 1000000000;
  int64_t nanoseconds 
      = (int64_t)(seconds * s_nNanoSecondsPerSecond);
  abstime.tv_sec 
      += (time_t)((abstime.tv_nsec + nanoseconds) / s_nNanoSecondsPerSecond);
  abstime.tv_nsec 
      = (long)((abstime.tv_nsec + nanoseconds) % s_nNanoSecondsPerSecond);
  abstime.tv_sec += seconds;
  // 在对象析构时，设置互斥锁的线程id为执行析构的线程
  // 对象构造时，设置互斥锁的线程id为０
  MutexLock::UnassignGuard ug(m_nMutex);
  return ETIMEDOUT == pthread_cond_timedwait(
          &m_nCond, 
          m_nMutex.getPthreadMutex(), 
          &abstime);
}

