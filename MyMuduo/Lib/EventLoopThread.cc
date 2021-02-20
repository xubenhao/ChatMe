#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(
    const ThreadInitCallback& cb,
    const string& name)
  : m_pLoop(NULL),
    m_bExiting(false),
    m_nThread(
        std::bind(
            &EventLoopThread::threadFunc,
            this), 
        name),
    m_nMutex(),
    m_nCond(m_nMutex),
    m_nCallBack(cb)
{
}

EventLoopThread::~EventLoopThread()
{
  m_bExiting = true;
  if (m_pLoop != NULL)
  {
    m_pLoop->quit();
    m_nThread.join();
  }
}

EventLoop* EventLoopThread::startLoop()
{
  assert(!m_nThread.started());
  m_nThread.start();
  EventLoop* loop = NULL;
  // 利用互斥锁＋条件变量实现线程间同步
  // 控制线程等待派生的新线程实际执行
  {
    MutexLockGuard lock(m_nMutex);
    while (m_pLoop == NULL)
    {
      m_nCond.wait();
    }

    loop = m_pLoop;
  }

  return loop;
}

// 以事件循环作为执行体的线程
void EventLoopThread::threadFunc()
{
  EventLoop loop;
  if (m_nCallBack)
  {
    m_nCallBack(&loop);
  }

  {
    MutexLockGuard lock(m_nMutex);
    m_pLoop = &loop;
    m_nCond.notify();
  }

  // 线程在loop里执行时，
  // this指向的EventLoop对象构成了线程执行的语境
  // EventLoop对象不仅派生线程以其作为语境
  // 该对象保存在控制线程的EventLoopThread对象内,
  // 且会被startLoop返回
  //
  // 任何可访问到线程语境的线程
  // 可以通过更改语境对象内容来与语境对象关联的线程交互
  // 交互包括：
  // 给线程提交回调任务
  // 给epoll添加／删除／更改关闭监视的描述符集合及事件信息
  loop.loop();
  MutexLockGuard lock(m_nMutex);
  m_pLoop = NULL;
}

