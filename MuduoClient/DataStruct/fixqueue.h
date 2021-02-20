//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030

#ifndef DATA_STRUCT_FIXQUEUE_H
#define DATA_STRUCT_FIXQUEUE_H
#include "header.h"
#include "doublelist.h"

namespace NDataStruct
{
    template <typename T, int N>
    class FixQueue
    {
    public:
        FixQueue();
        virtual ~FixQueue();

        FixQueue(const FixQueue& dqA_);
        FixQueue& operator=(const FixQueue& dqA_);

        void In(const T& nValue_);
        T Out();
        T Peek() const;
        bool IsEmpty() const;
        int Size() const;
        DoubleList<T> GetList() const
        {
            return m_List;
        }
    private:
        DoubleList<T> m_List;
    };

    template <typename T, int N>
    FixQueue<T,N>::FixQueue()
    {
    }

    template <typename T, int N>
    FixQueue<T,N>::~FixQueue()
    {
    }

    template <typename T, int N>
    FixQueue<T,N>::FixQueue(const FixQueue& dqA_)
    {
        m_List = dqA_.m_List;
    }

    template <typename T, int N>
    FixQueue<T,N>& FixQueue<T,N>::operator=(const FixQueue& dqA_)
    {
        if (this == &dqA_)
        {
            return *this;
        }

        m_List = dqA_.m_List;
        return *this;
    }

    template <typename T, int N>
    void FixQueue<T,N>::In(const T& nValue_)
    {
        if(Size() >= N)
        {
            return;
        }

        m_List.InsertLast(nValue_);
    }

    template <typename T, int N>
    T FixQueue<T,N>::Out()
    {
        if (IsEmpty())
        {
            throw "queue is empty";
        }

        typename DoubleList<T>::Node* _pFirst = m_List.GetFirst();
        T _nValue = _pFirst->GetValue();
        m_List.DeleteFirst();
        return _nValue;
    }

    template <typename T, int N>
    T FixQueue<T,N>::Peek() const
    {
        if (IsEmpty())
        {
            throw "queue is empty";
        }

        typename DoubleList<T>::Node* _pFirst = m_List.GetFirst();
        return _pFirst->GetValue();
    }

    template <typename T, int N>
    bool FixQueue<T,N>::IsEmpty() const
    {
        return m_List.IsEmpty();
    }

    template <typename T, int N>
    int FixQueue<T,N>::Size() const
    {
        return m_List.GetSize();
    }
}

#endif
