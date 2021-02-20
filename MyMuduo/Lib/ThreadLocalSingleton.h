#ifndef NLIB_THREADLOCALSINGLETON_H
#define NLIB_THREADLOCALSINGLETON_H
#include "header.h"

template<typename T>
class ThreadLocalSingleton 
{
    class Deleter
    {
    public:
        Deleter()
        {
          pthread_key_create(
            &m_nKey, 
            &ThreadLocalSingleton::destructor);
        }

        ~Deleter()
        {
          pthread_key_delete(m_nKey);
        }

        void set(T* newObj)
        {
          assert(pthread_getspecific(m_nKey) == NULL);
          pthread_setspecific(m_nKey, newObj);
        }

        pthread_key_t m_nKey;
    };


public:
    // 不可定义此模板类型的局部对象
    // 不可用new产生此模板类型对象
    ThreadLocalSingleton() = delete;
    ~ThreadLocalSingleton() = delete;
   
    // m_pValue每个线程共享一个静态值对象［指针］
    // 每个线程初次执行到此时，分配一个模板T动态对象，
    // 将m_pValue指向它
    //
    // 进程内所有线程针对模板类型T共享唯一一个静态值对象m_nDelete
    // 进程首次使用模板类型T时，触发一个全局唯一的m_nDelete的构造
    static T& instance()
    {
        if (!m_pValue)
        {
          m_pValue = new T();
          // 再将线程特定数据设置通过所有线程共享的索引
          // 设置到线程特定数据槽，
          // 有点多余．
          // 利用__thread修饰的线程特定数据自身可以实现此功能
          m_nDeleter.set(m_pValue);
        }
        
        return *m_pValue;
    }

    static T* pointer()
    {
        return m_pValue;
    }
    
private:
    static void destructor(void* obj)
    {
        delete m_pValue;
        m_pValue = 0;
    }
    
private:
    static __thread T* m_pValue;
    static Deleter m_nDeleter;
};

// 每个线程内所有类型T的模板类对象共享一个m_pValue
template<typename T>
__thread T* ThreadLocalSingleton<T>::m_pValue = 0;

// 所有类型T的模板类对象共享一个m_nDeleter
template<typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::m_nDeleter;

// 因为delete的构造／析构，不会存在此模板类的对象
// 这样，
// 每个线程，对类型T，
// 通过模板类的instance静态调用可获得调用线程特定的一个T对象
// instance静态方法，若调用线程的T对象尚不存在，
// 动态产生对象后，既将对象指针存储在调用线程特定的m_pValue又通过m_nDeleter存储在线程特定数据
// 多余．
// m_pValue本身已经有了线程特定数据的意义，这时，不必再用m_nDelete来设置线程特定数据．
// 通过模板类的pointer静态调用可访问调用线程特定的一个T对象［若存在的话］
#endif

