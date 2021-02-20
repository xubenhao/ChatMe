//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030

#ifndef DATA_STRUCT_DYNQUEUE_H
#define DATA_STRUCT_DYNQUEUE_H
#include "header.h"
#include "doublelist.h"

namespace NDataStruct
{
    template <typename T>
    class DynQueue
    {
    public:
        DynQueue();
        virtual ~DynQueue();

        DynQueue(const DynQueue& dqA_);
        DynQueue& operator=(const DynQueue& dqA_);

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

    template <typename T>
    DynQueue<T>::DynQueue()
    {
    }

    template <typename T>
    DynQueue<T>::~DynQueue()
    {
    }

    template <typename T>
    DynQueue<T>::DynQueue(const DynQueue& dqA_)
    {
        m_List = dqA_.m_List;
    }

    template <typename T>
    DynQueue<T>& DynQueue<T>::operator=(const DynQueue& dqA_)
    {
        if (this == &dqA_)
        {
            return *this;
        }

        m_List = dqA_.m_List;
        return *this;
    }

    template <typename T>
    void DynQueue<T>::In(const T& nValue_)
    {
        m_List.InsertLast(nValue_);
    }

    template <typename T>
    T DynQueue<T>::Out()
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

    template <typename T>
    T DynQueue<T>::Peek() const
    {
        if (IsEmpty())
        {
            throw "queue is empty";
        }

        typename DoubleList<T>::Node* _pFirst = m_List.GetFirst();
        return _pFirst->GetValue();
    }

    template <typename T>
    bool DynQueue<T>::IsEmpty() const
    {
        return m_List.IsEmpty();
    }

    template <typename T>
    int DynQueue<T>::Size() const
    {
        return m_List.GetSize();
    }
}

#endif
