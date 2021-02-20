#include "Thread.h"
#include "CurrentThread.h"
#include "Exception.h"
#include "Logging.h"

void afterFork()
{
    // 项目里未见到fork
    // 这里应该没机会机制
    printf("unexpect case of fork\n");
    t_cachedTid = 0;
    t_threadName = "main";
    tid();
}

class ThreadNameInitializer
{
public:
    ThreadNameInitializer()
    {
        t_threadName = "main";
        tid();
        //pthread_atfork()在fork()之前调用，
        //当调用fork时，
        //内部创建子进程前在父进程中会调用参数１，
        //内部创建子进程成功后，
        //父进程会调用参数２，
        //子进程会调用参数３。 
        pthread_atfork(
            NULL, 
            NULL, 
            &afterFork);
    }
};

// 所有线程共享的全局对象［所有线程可见］
ThreadNameInitializer init;

struct ThreadData
{
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc m_nFunc;
    string m_strName;
    pid_t* m_pTid;
    CountDownLatch* m_pLatch;
    ThreadData(
        ThreadFunc func,
        const string& name,
        pid_t* tid,
        CountDownLatch* latch)
        : m_nFunc(std::move(func)),
        m_strName(name),
        m_pTid(tid),
        m_pLatch(latch)
    { 
    }

    // 线程函数
    // 通过成员函数形式
    // 而非全局函数来执行函数回调
    // 或者
    // 在成员函数内执行C的API的好处是：
    // 借助于成员函数内隐含的this
    // 获得执行时一个对象
    // 此对象构成执行回调／API时的语境，
    // 可以提供一些所需的实时信息
    // 这些信息可以作为输入，
    // 也可以在执行函数调用前后修改，
    // 一般外部通过this指向对象获得修改后信息
    void runInThread()
    {
        *m_pTid = tid();
        m_pTid = NULL;
        m_pLatch->countDown();
        m_pLatch = NULL;
        t_threadName = 
            m_strName.empty() ? "muduoThread" : m_strName.c_str();
        ::prctl(
            PR_SET_NAME, 
            t_threadName);
        
        try
        {
          m_nFunc();
          t_threadName = "finished";
        }
        catch (const Exception& ex)
        {
          t_threadName = "crashed";
          fprintf(
            stderr, 
            "exception caught in Thread %s\n", 
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
          t_threadName = "crashed";
          fprintf(
            stderr, 
            "exception caught in Thread %s\n", 
            m_strName.c_str());
          fprintf(
            stderr, 
            "reason: %s\n", 
            ex.what());
          abort();
        }
        catch (...)
        {
          t_threadName = "crashed";
          fprintf(
            stderr, 
            "unknown exception caught in Thread %s\n", 
            m_strName.c_str());
          throw; 
        }
    }
};

// 属于全局回调
void* startThread(void* obj)
{
    // 在这里已经属于新线程的执行环境了
    ThreadData* data = (ThreadData*)(obj);
    // 增加了一层间接性
    // 通过线程回调参数执行的对象的成员函数
    // 来触发实际线程回调
    // 实际线程回调及相关语境信息
    // 均作为这里线程参数指向对象的数据成员提供
    data->runInThread();
    delete data;
    return NULL;
}

// 所有线程共享的全局对象
// 可以被所有线程共享，
// 因为该对象对所有线程均是可见的
// 自身均被多线程互斥保证，无需额外互斥锁来保证
AtomicInt32 Thread::s_nNumCreated;
Thread::Thread(
    ThreadFunc func, 
    const string& n)
    : m_bStarted(false),
    m_bJoined(false),
    m_nPthreadId(0),
    m_nTid(0),
    m_nFunc(std::move(func)),
    m_strName(n),
    m_nLatch(1)
{
    setDefaultName();
}

Thread::~Thread()
{
    if (m_bStarted && !m_bJoined)
    {
        pthread_detach(m_nPthreadId);
    }
}

void Thread::setDefaultName()
{
    int num = s_nNumCreated.incrementAndGet();
    if (m_strName.empty())
    {
        char buf[32];
        snprintf(
            buf, 
            sizeof buf, 
            "Thread%d", 
            num);
        m_strName = buf;
    }
}

// 在类的成员函数里执行C的API
// 执行时隐含的this指向对象构成了执行时的语境
// 执行前后可以从语境获取信息，设置语境里的信息
//
// 通过语境对象，其他时机／其他线程可访问到语境信息
void Thread::start()
{
    assert(!m_bStarted);
    m_bStarted = true;
    ThreadData* data = new ThreadData(
            m_nFunc, 
            m_strName, 
            &m_nTid, // 为了得到pid_t结构保存的线程id
            &m_nLatch);
    if (pthread_create(
            &m_nPthreadId,// pthread_t结构保存的线程id 
            NULL, 
            startThread, 
            data))
    {
        m_bStarted = false;
        delete data; 
        LOG_SYSFATAL 
            << "Failed in pthread_create";
    }
    else
    {
        // 阻塞等待，直到新线程实际执行
        m_nLatch.wait();
        assert(m_nTid > 0);
    }
}

int Thread::join()
{
    assert(m_bStarted);
    assert(!m_bJoined);
    m_bJoined = true;
    return pthread_join(m_nPthreadId, NULL);
}

