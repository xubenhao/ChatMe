#ifndef NLIB_ATOMIC_H
#define NLIB_ATOMIC_H

#include "header.h"
template<typename T>
class AtomicIntegerT 
{
public:
    AtomicIntegerT()
        : m_nValue(0)
    {
    }

    T get()
    {
        return __sync_val_compare_and_swap(&m_nValue, 0, 0);
    }
          
    T getAndAdd(T x)
    {
        return __sync_fetch_and_add(&m_nValue, x);
    }

    T addAndGet(T x)
    {
        return getAndAdd(x) + x;
    }

    T incrementAndGet()
    {
        return addAndGet(1);
    }

    T decrementAndGet()
    {
        return addAndGet(-1);
    }

    void add(T x)
    {
        getAndAdd(x);
    }

    void increment()
    {
        incrementAndGet();
    }
          
    void decrement()
    {
        decrementAndGet();
    }

    T getAndSet(T newValue)
    {
        return __sync_lock_test_and_set(&m_nValue, newValue);
    }

private:
    volatile T m_nValue;
};

typedef AtomicIntegerT<int32_t> AtomicInt32;
typedef AtomicIntegerT<int64_t> AtomicInt64;
#endif

