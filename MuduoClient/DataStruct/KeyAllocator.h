//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030
//

#ifndef DATA_STRUCT_KEYALLOCATOR_H
#define DATA_STRUCT_KEYALLOCATOR_H
#include "header.h"
#include "dynarray.h"
namespace NDataStruct
{
    template <typename T>
    class KeyAllocator
    {
    public:
        class Node
        {
        public:
            Node(T& nData_)
            {
                m_bValid = true;
                m_nData = nData_;
            }

            Node()
            {
                m_bValid = false;
            }

        public:
            T m_nData;
            bool m_bValid;
        };

        KeyAllocator()
        {
            m_nNextKey = 0;
        }

        ~KeyAllocator()
        {

        }

        void Reset()
        {
            m_nNextKey = 0;
            m_arrMap.DeleteAll();
        }

        int Allocate()
        {
            /*if(m_nNextKey == N-1)
            {
                assert(false);
                return -1;
            }*/
            m_arrMap.Add(Node());
            return m_nNextKey++;
        }

        bool Register(int nIndex_, T& nData_)
        {
            if(nIndex_ < 0 || nIndex_ >= m_nNextKey)
            {
                throw "Index error";
            }

            if(m_arrMap[nIndex_].m_bValid == false)
            {
                Node _nNode = Node(nData_);
                m_arrMap[nIndex_] = _nNode;
                return true;
            }
            else
            {
                return false;
            }
        }

        bool Find(int nIndex_, T& nData_)
        {
            if(nIndex_ < 0 || nIndex_ >= m_nNextKey)
            {
                throw "Index error";
            }

            if(m_arrMap[nIndex_].m_bValid == false)
            {
                return false;
            }
            else
            {
                nData_ = m_arrMap[nIndex_].m_nData;
                return true;
            }
        }

        bool Find(const T& nData_, int& nIndex_)
        {
            for(int i = 0; i < m_nNextKey; i++)
            {
                if(m_arrMap[i].m_bValid && m_arrMap[i].m_nData == nData_)
                {
                    nIndex_ = i;
                    return true;
                }
            }

            return false;
        }
    private:
        int m_nNextKey;
        NDataStruct::DynArray<Node> m_arrMap;
    };
}

#endif // KEYALLOCATOR_H
