#ifndef NLIB_THREADPOOL_H
#define NLIB_THREADPOOL_H

#include "header.h"
#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"

// 1.通过类成员函数启动多个线程＋
// 线程回调设置为类的成员函数好处
//
// 一个控制线程通过线程池对象
// 构造多个Thread对象，并启动多个并发执行的线程
//
// 由于线程池对象所启动的线程
// 最后的线程函数是线程池类型的成员函数
// 且std::bind时绑定了this
//
// 所有线程池启动的每个线程均可通过
// 隐式的this获得线程需要的语境．
//
// 线程池对象的成员既包含由其启动的线程执行所需要的语境信息
// 也包含它启动的每个线程的属性信息，
// 这些信息对线程池启动的线程可能没用．
// 但对于可访问到线程池对象的访问者，
// 可以提供线程池所管理的所有线程的属性，状态，
// 这些信息可能正是访问者需要的信息
//
// ２．线程池工作模型
// 线程池内每个线程可访问到一个共享的线程池对象
// 所有需要提交执行任务＆获取线程池属性／状态的访问者
// 也可以访问到共享的线程池对象
//
//
// 访问者，可以提交任务
// 线程池内每个执行线程，
// 要么，在执行任务
// 要么，取出一个任务并执行
// 要么，阻塞，等待有任务可取
//
// 任务队列存储在多个线程可共享的线程池对象内
// 线程池中每个线程对任务队列来说，是消费者
// 任何需借助线程池对象提交任务的访问线程，是生产者
//
//
// 这样形成了一个
// 多线程下对共享资源的多生产者多消费者问题
// 需要线程间互斥／同步机制．
//
class ThreadPool  
{
public:
    typedef std::function<void ()> Task;
    explicit ThreadPool(
        const string& nameArg = string("ThreadPool"));
    ~ThreadPool();
    void setMaxQueueSize(int maxSize) 
    { 
        m_nMaxQueueSize = maxSize; 
    }
    
    void setThreadInitCallback(const Task& cb)
    { 
        m_nThreadInitCallback = cb; 
    }

    void start(int numThreads);
    void stop();
    const string& name() const
    { 
        return m_strName; 
    }

    size_t queueSize() const;
    void run(Task f);

private:
    bool isFull() const;
    void runInThread();
    Task take();
    
private:
    mutable MutexLock m_nMutex;
    Condition m_nNotEmpty;
    Condition m_nNotFull;
    
    string m_strName;
    Task m_nThreadInitCallback;
    std::vector<std::unique_ptr<Thread>> m_vecThreads;
    std::deque<Task> m_nQueue;
    size_t m_nMaxQueueSize;
    bool m_bRunning;
};


#endif

