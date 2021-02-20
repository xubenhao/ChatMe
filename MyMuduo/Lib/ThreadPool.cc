#include "ThreadPool.h"
#include "Exception.h"

ThreadPool::ThreadPool(
    const string& nameArg)
  : m_nMutex(),
    m_nNotEmpty(m_nMutex),
    m_nNotFull(m_nMutex),
    m_strName(nameArg),
    m_nMaxQueueSize(0),
    m_bRunning(false)
{
}

ThreadPool::~ThreadPool()
{
  if (m_bRunning)
  {
    stop();
  }
}

void ThreadPool::start(int numThreads)
{
  assert(m_vecThreads.empty());
  m_bRunning = true;
  m_vecThreads.reserve(numThreads);
  for (int i = 0; i < numThreads; ++i)
  {
    char id[32];
    snprintf(
        id, 
        sizeof(id), 
        "%d", 
        i+1);
    m_vecThreads.emplace_back(
        new Thread(
            std::bind(
                &ThreadPool::runInThread, 
                this), 
            m_strName+id));
    m_vecThreads[i]->start();
  }

  if (numThreads == 0 
    && m_nThreadInitCallback)
  {
    m_nThreadInitCallback();
  }
}

void ThreadPool::stop()
{
  {
    MutexLockGuard lock(m_nMutex);
    m_bRunning = false;
    m_nNotEmpty.notifyAll();
  }
  
  for (auto& thr : m_vecThreads)
  {
    thr->join();
  }
}

size_t ThreadPool::queueSize() const
{
  MutexLockGuard lock(m_nMutex);
  return m_nQueue.size();
}

void ThreadPool::run(Task task)
{
  if (m_vecThreads.empty())
  {
    task();
  }
  else
  {
    MutexLockGuard lock(m_nMutex);
    while (isFull())
    {
      m_nNotFull.wait();
    }
    
    assert(!isFull());
    m_nQueue.push_back(std::move(task));
    m_nNotEmpty.notify();
  }
}

ThreadPool::Task ThreadPool::take()
{
  MutexLockGuard lock(m_nMutex);
  while (m_nQueue.empty() && m_bRunning)
  {
    m_nNotEmpty.wait();
  }

  Task task;
  if (!m_nQueue.empty())
  {
    task = m_nQueue.front();
    m_nQueue.pop_front();
    if (m_nMaxQueueSize > 0)
    {
      m_nNotFull.notify();
    }
  }

  return task;
}

bool ThreadPool::isFull() const
{
  m_nMutex.assertLocked();
  return m_nMaxQueueSize > 0 
      && m_nQueue.size() >= m_nMaxQueueSize;
}

void ThreadPool::runInThread()
{
  try
  {
    if (m_nThreadInitCallback)
    {
      m_nThreadInitCallback();
    }

    while (m_bRunning)
    {
      Task task(take());
      if (task)
      {
        task();
      }
    }
  }
  catch (const Exception& ex)
  {
    fprintf(
        stderr, 
        "exception caught in ThreadPool %s\n", 
        m_strName.c_str());
    fprintf(
        stderr, 
        "reason: %s\n", 
        ex.what());
    fprintf(
        stderr, 
        "stack trace: %s\n", 
        ex.stackTrace());
    abort();
  }
  catch (const std::exception& ex)
  {
    fprintf(
        stderr, 
        "exception caught in ThreadPool %s\n", 
        m_strName.c_str());
    fprintf(
        stderr, 
        "reason: %s\n", 
        ex.what());
    abort();
  }
  catch (...)
  {
    fprintf(
        stderr, 
        "unknown exception caught in ThreadPool %s\n", 
        m_strName.c_str());
    throw; 
  }
}

