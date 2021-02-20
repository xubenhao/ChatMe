#include "Timer.h"

AtomicInt64 Timer::s_nNumCreated;

void Timer::restart(
        TimeStamp now)
{
  if (m_bRepeat)
  {
    m_nExpiration = 
        addTime(now, m_nInterval);
  }
  else
  {
    m_nExpiration = 
        TimeStamp::invalid();
  }
}


