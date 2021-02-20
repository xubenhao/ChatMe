#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "Logging.h"

// 创建线程池对象的线程可任务是管理线程
// 由其通过线程池对象接口创建的多个线程可视为工作者线程
//
// 线程池对象持有的EventLoopThread对象集合，可以用来
// １．获得每个启动线程的信息
// ２．与每个启动的线程的交互，
// 由于这里的线程都是以事件循环作为执行体的线程，
// 所以与其交互的方式有
// ａ．向其提交回调请求
// ｂ．增加／删除／更改事件循环epoll注册的描述符及描述符的事件
// ｃ．控制线程停止／．．．
EventLoopThreadPool::EventLoopThreadPool(
    EventLoop* baseLoop,
    const string& nameArg)
  : m_pBaseLoop(baseLoop), 
    m_strName(nameArg),
    m_bStarted(false),
    m_nNumThreads(0),
    m_nNext(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{

}

void EventLoopThreadPool::start(
    const ThreadInitCallback& cb)
{
  assert(!m_bStarted);
  m_pBaseLoop->assertInLoopThread();
  m_bStarted = true;
  LOG_TRACE
      << "The thread number in thread pool is"
      << m_nNumThreads;
  for (int i = 0; i < m_nNumThreads; ++i)
  {
    char buf[m_strName.size() + 32];
    snprintf(
        buf, 
        sizeof buf, 
        "%s%d", 
        m_strName.c_str(), 
        i);
    EventLoopThread* t = new EventLoopThread(
            cb, 
            buf);
    // 这里利用unique_ptr来管理动态资源
    m_vecThreads.push_back(
            std::unique_ptr<EventLoopThread>(t));
    
    // 既保存EventLoopThread
    // 又保存EventLoop对象集合是否多余?
    m_vecLoops.push_back(
            t->startLoop());
  }

  if (m_nNumThreads == 0 && cb)
  {
    cb(m_pBaseLoop);
  }
}

// 多余的EventLoop对象集合一定程度上
// 遍历了对EventLoop的访问－－少了一个中间层
EventLoop* EventLoopThreadPool::getNextLoop()
{
  m_pBaseLoop->assertInLoopThread();
  assert(m_bStarted);
  EventLoop* loop = m_pBaseLoop;
  if (!m_vecLoops.empty())
  {
    loop = m_vecLoops[m_nNext];
    ++m_nNext;
    if ((size_t)(m_nNext) >= m_vecLoops.size())
    {
      m_nNext = 0;
    }
  }

  return loop;
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode)
{
  m_pBaseLoop->assertInLoopThread();
  EventLoop* loop = m_pBaseLoop;
  if (!m_vecLoops.empty())
  {
    loop = m_vecLoops[hashCode % m_vecLoops.size()];
  }

  return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
  m_pBaseLoop->assertInLoopThread();
  assert(m_bStarted);
  if (m_vecLoops.empty())
  {
    return std::vector<EventLoop*>(1, m_pBaseLoop);
  }
  else
  {
    return m_vecLoops;
  }
}

