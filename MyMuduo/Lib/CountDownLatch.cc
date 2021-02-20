#include "CountDownLatch.h"

CountDownLatch::CountDownLatch(int count)
  : m_nMutex(),
    m_nCondition(m_nMutex),
    m_nCount(count)
{
}

void CountDownLatch::wait()
{
  MutexLockGuard lock(m_nMutex);
  while (m_nCount > 0)
  {
    m_nCondition.wait();
  }
}

void CountDownLatch::countDown()
{
  MutexLockGuard lock(m_nMutex);
  --m_nCount;
  if (m_nCount == 0)
  {
    m_nCondition.notifyAll();
  }
}

int CountDownLatch::getCount() const
{
  MutexLockGuard lock(m_nMutex);
  return m_nCount;
}

