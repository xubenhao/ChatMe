//	Author : XuBenHao
//	Version : 1.0.0
//	Mail : xbh370970843@163.com
//	Copyright : XuBenHao 2020 - 2030

#ifndef DATA_STRUCT_RED_BLACK_TREE_H
#define DATA_STRUCT_RED_BLACK_TREE_H
#include "header.h"
#include "dynarray.h"
#include "dynqueue.h"

namespace NDataStruct
{
    template<typename Key, typename Value>
    class RedBlackTree
    {
    public:
        enum Color
        {
            BLACK = 1,
            RED,
            BLACKBLACK,
        };

        class Pair
        {
        public:
            Pair()
            {
            }

            Pair(const Key& key_, const Value& value_)
            {
                m_nKey = key_;
                m_nValue = value_;
            }

            ~Pair()
            {
            }

        public:
            Key m_nKey;
            Value m_nValue;
        };

        class Node
        {
        public:
            Pair GetPair()
            {
                return m_nPair;
            }

            Node()
            {
                m_pParent = nullptr;
                m_pLeftChild = nullptr;
                m_pRightChild = nullptr;
                m_nColor = RED;
            }

            Node(const Pair& nPair_)
            {
                m_nPair = nPair_;
                m_pParent = nullptr;
                m_pLeftChild = nullptr;
                m_pRightChild = nullptr;
                m_nColor = RED;
            }

            ~Node()
            {
            }

        private:
            Node* m_pParent;
            Node* m_pLeftChild;
            Node* m_pRightChild;
            Pair m_nPair;
            Color m_nColor;
            friend class RedBlackTree;
        };

        RedBlackTree()
        {
            m_pRoot = nullptr;
        }

        ~RedBlackTree()
        {
            Reset();
        }

        void Reset()
        {
            DynArray<Node*> _arrNodes;
            PreVisit([&_arrNodes](Node* pNode_)
            {
                _arrNodes.Add(pNode_);
            });

            for (int _i = 0; _i < _arrNodes.GetSize(); _i++)
            {
                delete _arrNodes[_i];
                _arrNodes[_i] = nullptr;
            }

            m_pRoot = nullptr;
        }

        RedBlackTree(const RedBlackTree& minTree_);
        RedBlackTree& operator=(const RedBlackTree& minTree_);


        void PreVisit(std::function<void(Node*)> nFun_)
        {
            if(m_pRoot == NULL)
            {
                return;
            }

            nFun_(m_pRoot);
            PreVisit(nFun_, m_pRoot->m_pLeftChild);
            PreVisit(nFun_, m_pRoot->m_pRightChild);
        }

        void MidVisit(std::function<void(Node*)> nFun_)
        {
            if(m_pRoot == NULL)
            {
                return;
            }

            MidVisit(nFun_, m_pRoot->m_pLeftChild);
            nFun_(m_pRoot);
            MidVisit(nFun_, m_pRoot->m_pRightChild);
        }

        void PostVisit(std::function<void(Node*)> nFun_)
        {
            if(m_pRoot == NULL)
            {
                return;
            }

            PostVisit(nFun_, m_pRoot->m_pLeftChild);
            PostVisit(nFun_, m_pRoot->m_pRightChild);
            nFun_(m_pRoot);
        }

        void LevelVisit(std::function<void(Node*)> nFun_)
        {
            // 创建队列
            // 根结点入队列
            // 循环迭代：
            // 出队列＋结点访问
            // 左孩子若存在，则入队列
            // 右孩子若存在，则入队列
            //
            // 循环迭代终止条件：
            // 队列为空
            DynQueue<Node*> _nQueue;
            _nQueue.In(m_pRoot);
            while(_nQueue.IsEmpty() == false)
            {
                Node* _pNode = _nQueue.Out();
                nFun_(_pNode);
                if(_pNode->m_pLeftChild)
                {
                    _nQueue.In(_pNode->m_pLeftChild);
                }

                if(_pNode->m_pRightChild)
                {
                    _nQueue.In(_pNode->m_pRightChild);
                }
            }
        }

        Node* Find(const Key& nKey_)
        {
            if(m_pRoot == NULL)
            {
                return NULL;
            }

            if(m_pRoot->m_nPair.m_nKey == nKey_)
            {
                return m_pRoot;
            }
            else if(m_pRoot->m_nPair.m_nKey < nKey_)
            {
                return Find(nKey_, m_pRoot->m_pRightChild);
            }
            else
            {
                return Find(nKey_, m_pRoot->m_pLeftChild);
            }
        }

        bool FindMin(Pair& nPair_, Node** ppNode_ = NULL)
        {
            if(IsEmpty())
            {
                return false;
            }

            if(m_pRoot->m_pLeftChild)
            {
                FindMin(nPair_, ppNode_, m_pRoot->m_pLeftChild);
                return true;
            }
            else
            {
                nPair_ = m_pRoot->m_nPair;
                if(ppNode_)
                {
                    *ppNode_ = m_pRoot;
                }

                return true;
            }
        }

        bool FindMax(Pair& nPair_, Node** ppNode_ = NULL)
        {
            if(IsEmpty())
            {
                return false;
            }

            if(m_pRoot->m_pRightChild)
            {
                FindMax(nPair_, ppNode_, m_pRoot->m_pRightChild);
                return true;
            }
            else
            {
                nPair_ = m_pRoot->m_nPair;
                if(ppNode_)
                {
                    *ppNode_ = m_pRoot;
                }

                return true;
            }
        }


        bool IsEmpty()
        {
            return m_pRoot == NULL;
        }

        // For Iteration
        Node* Pre(Node* pNode_)
        {
            if(pNode_ == NULL)
            {
                throw "input error";
            }

            Node* _pTemp = Find(pNode_->m_nPair.m_nKey);
            if(_pTemp == NULL)
            {
                throw "input error";
            }

            if(pNode_->m_pLeftChild)
            {
                Pair _nPair;
                Node* _pMax = NULL;
                FindMax(_nPair, &_pMax, pNode_->m_pLeftChild);
                assert(_pMax);
                return _pMax;
            }

            // 结点左孩子不存在
            // 结点是以其为根子树中最小的结点
            // 循环迭代：
            // 若结点父亲不存在，结点是树中最小的结点，前驱结点不存在．结束迭代
            // 若结点是父亲左孩子，结点是父亲结点为根子树中的最小结点.将迭代结点设为父结点．
            // 若结点是父亲右孩子，父亲结点是结点的上一结点．结束迭代
            //
            // 循环不变式：
            // 在迭代结点为根子树中，参数结点是树中最小的结点
            Node* _pNode = pNode_;
            while(true)
            {
                Node* _pParent = _pNode->m_pParent;
                if(_pParent == NULL)
                {
                    return NULL;
                }

                if(_pParent->m_pLeftChild == _pNode)
                {
                    _pNode = _pParent;
                }
                else
                {
                    assert(_pParent->m_pRightChild == _pNode);
                    return _pParent;
                }

            }
        }

        Node* Next(Node* pNode_)
        {
            if(pNode_ == NULL)
            {
                throw "input error";
            }

            Node* _pTemp = Find(pNode_->m_nPair.m_nKey);
            if(_pTemp == NULL)
            {
                throw "input error";
            }

            if(pNode_->m_pRightChild)
            {
                Pair _nPair;
                Node* _pMin = NULL;
                FindMin(_nPair, &_pMin, pNode_->m_pRightChild);
                //assert(_bRet && _pMin);
                return _pMin;
            }

            // todo
            // 如果结点的右孩子存在，则右孩子子树中最小的结点就是下一结点
            // 如结点的右孩子不存在，该结点是以其为根子树中最大的结点
            // 循环迭代
            // 如该结点父亲存在且是父亲左孩子，则其父亲是其下一结点，结束
            // 如该结点父亲存在且是父亲右孩子，则其该结点是其父亲结点为根子树中最大结点．对父亲结点继续循环迭代
            // 如该结点父亲不存在，该结点是树的最大结点，在树中其下一结点不存在
            // 循环不变式：
            // 参数结点是循环迭代结点为根子树中最大的那个结点
            Node* _pNode = pNode_;
            while(true)
            {
                 Node* _pParent = _pNode->m_pParent;
                 if(_pParent == NULL)
                 {
                    return NULL;
                 }

                 if(_pParent->m_pLeftChild == _pNode)
                 {
                    return _pParent;
                 }
                 else
                 {
                    assert(_pParent->m_pRightChild == _pNode);
                    _pNode = _pParent;
                 }
            }
        }

        Node* Begin()
        {
            Pair _nPair;
            Node* _pMin = NULL;
            bool _bRet = FindMin(_nPair, &_pMin);
            assert(_bRet && _pMin);
            return _pMin;
        }

        Node* Last()
        {
            Pair _nPair;
            Node* _pMax = NULL;
            bool _bRet = FindMax(_nPair, &_pMax);
            assert(_bRet && _pMax);
            return _pMax;
        }

        bool Add(
                const Pair& nPair_)
        {
            Node* _pNode = Find(nPair_.m_nKey);
            if(_pNode)
            {
                return false;
            }

            // Find Insert Pos
            _pNode = new Node(nPair_);
            if(m_pRoot == NULL)
            {
                _pNode->m_nColor = BLACK;
                m_pRoot = _pNode;
                return true;
            }

            Node* _pTemp = m_pRoot;
            // Find insert position and insert
            while(true)
            {
                if(_pTemp->m_nPair.m_nKey < nPair_.m_nKey)
                {
                    // must insert in _ptemp's right child tree
                    if(_pTemp->m_pRightChild)
                    {
                        _pTemp = _pTemp->m_pRightChild;
                    }
                    else
                    {
                        // Find insert pos
                        _pNode->m_pParent = _pTemp;
                        _pTemp->m_pRightChild = _pNode;
                        break;
                    }
                }
                else
                {
                    if(_pTemp->m_pLeftChild)
                    {
                        _pTemp = _pTemp->m_pLeftChild;
                    }
                    else
                    {
                        _pNode->m_pParent = _pTemp;
                        _pTemp->m_pLeftChild = _pNode;
                        break;
                    }
                }
            }

            if (_pNode->m_pParent->m_nColor == RED)
            {
                AdjustForParentColor(_pNode);
            }

            return true;
        }

        void Delete(const Key& nKey_)
        {
            Node* _pNode = Find(nKey_);
            if(_pNode == NULL)
            {
                return;
            }

            if(_pNode->m_pLeftChild == NULL
                && _pNode->m_pRightChild == NULL)
            {
                Node* _pParent = _pNode->m_pParent;
                if(_pParent == NULL)
                {
                    delete _pNode;
                    m_pRoot = NULL;
                }
                else
                {
                    if(_pParent->m_pLeftChild == _pNode)
                    {
                        _pParent->m_pLeftChild = NULL;
                    }
                    else
                    {
                        assert(_pParent->m_pRightChild == _pNode);
                        _pParent->m_pRightChild = NULL;
                    }

                    if(_pNode->m_nColor == BLACK)
                    {
                        AdjustForBlackHeight(_pParent);
                    }

                    delete _pNode;
                }
            }
            else if(_pNode->m_pLeftChild)
            {
                Pair _nPair;
                Node* _pMaxNode = NULL;
                FindMax(_nPair, &_pMaxNode, _pNode->m_pLeftChild);
                //assert(_bRet);
                _pNode->m_nPair = _nPair;
                Delete(_nPair.m_nKey, _pMaxNode);
            }
            else
            {
                assert(_pNode->m_pRightChild);
                Pair _nPair;
                Node* _pMinNode = NULL;
                FindMin(_nPair, &_pMinNode, _pNode->m_pRightChild);
                //assert(_bRet);
                _pNode->m_nPair = _nPair;
                Delete(_nPair.m_nKey, _pMinNode);
            }
        }

        void DeleteAll()
        {
            Reset();
        }

        Node* GetRoot() const
        {
            return m_pRoot;
        }

        DynArray<Pair> GetArray() const
        {
            DynArray<Pair> _arrContain;
            PreVisit([&_arrContain](Node* pNode_)
            {
                _arrContain.Add(pNode_->m_nPair);
            });

            return _arrContain;
        }

    private:
        void PreVisit(std::function<void(Node*)> nFun_, Node* pRoot_)
        {
            if(pRoot_ == NULL)
            {
                return;
            }

            nFun_(pRoot_);
            PreVisit(nFun_, pRoot_->m_pLeftChild);
            PreVisit(nFun_, pRoot_->m_pRightChild);
        }

        void MidVisit(std::function<void(Node*)> nFun_, Node* pRoot_)
        {
            if(pRoot_ == NULL)
            {
                return;
            }

            MidVisit(nFun_, pRoot_->m_pLeftChild);
            nFun_(pRoot_);
            MidVisit(nFun_, pRoot_->m_pRightChild);
        }

        void PostVisit(std::function<void(Node*)> nFun_, Node* pRoot_)
        {
            if(pRoot_ == NULL)
            {
                return;
            }

            PostVisit(nFun_, pRoot_->m_pLeftChild);
            PostVisit(nFun_, pRoot_->m_pRightChild);
            nFun_(pRoot_);
        }


        Node* Find(const Key& nKey_, Node* pRoot_)
        {
            if(pRoot_ == NULL)
            {
                return NULL;
            }

            if(pRoot_->m_nPair.m_nKey == nKey_)
            {
                return pRoot_;
            }
            else if(pRoot_->m_nPair.m_nKey < nKey_)
            {
                return Find(nKey_, pRoot_->m_pRightChild);
            }
            else
            {
                return Find(nKey_, pRoot_->m_pLeftChild);
            }
        }

        void FindMin(Pair& nPair_, Node** ppNode_,  Node* pRoot_)
        {
            assert(pRoot_);
            if(pRoot_->m_pLeftChild)
            {
                FindMin(nPair_, ppNode_,  pRoot_->m_pLeftChild);
            }
            else
            {
                nPair_ = pRoot_->m_nPair;
                if(ppNode_)
                {
                    *ppNode_ = pRoot_;
                }
            }
        }

        void FindMax(Pair& nPair_, Node** ppNode_, Node* pRoot_)
        {
            assert(pRoot_);
            if(pRoot_->m_pRightChild)
            {
                FindMax(nPair_, ppNode_, pRoot_->m_pRightChild);
            }
            else
            {
                nPair_ = pRoot_->m_nPair;
                if(ppNode_)
                {
                    *ppNode_ = pRoot_;
                }
            }
        }

        void Delete(const Key& nKey_, Node* pRoot_)
        {
            assert(pRoot_ && pRoot_->m_nPair.m_nKey == nKey_);
            Node* _pNode = pRoot_;
            if(_pNode->m_pLeftChild == NULL
                && _pNode->m_pRightChild == NULL)
            {
                Node* _pParent = _pNode->m_pParent;
                assert(_pParent);
                if(_pParent->m_pLeftChild == _pNode)
                {
                    _pParent->m_pLeftChild = NULL;
                }
                else
                {
                    assert(_pParent->m_pRightChild == _pNode);
                    _pParent->m_pRightChild = NULL;
                }

                if(_pNode->m_nColor == BLACK)
                {
                    AdjustForBlackHeight(_pParent);
                }

                delete _pNode;

            }
            else if(_pNode->m_pLeftChild)
            {
                Pair _nPair;
                Node* _pMaxNode = NULL;
                FindMax(_nPair, &_pMaxNode, _pNode->m_pLeftChild);
                _pNode->m_nPair = _nPair;
                Delete(_nPair.m_nKey, _pMaxNode);
            }
            else
            {
                assert(_pNode->m_pRightChild);
                Pair _nPair;
                Node* _pMinNode = NULL;
                FindMin(_nPair, &_pMinNode, _pNode->m_pRightChild);
                _pNode->m_nPair = _nPair;
                Delete(_nPair.m_nKey, _pMinNode);
            }
        }

        bool AdjustForParentColor(Node *pAdjustedNode_);
        Node* LeftRotate(Node *pNode_);
        Node* RightRotate(Node *pNode_);
        void AdjustForBlackBlack(Node *pBlackBlackNode_);
        void AdjustForBlackHeight(Node *pNode_);

        // 正确性测试支持：
        // 检查 红黑树合法性
        // 根黑
        // 同一节点不同分支黑高一致
        // 父红，则子黑
        // 颜色必须 为 红或黑
        bool CheckValid() const;
        int GetBlackHeight(Node *pNode_) const;
    private:
        Node* m_pRoot;
    };


    template<typename Key, typename Value>
    RedBlackTree<Key, Value>::RedBlackTree(
        const RedBlackTree& nTree_)
    {
        m_pRoot = nullptr;
        DynArray<Pair> _arrPairs = nTree_.GetArray();
        for (int _i = 0; _i < _arrPairs.GetSize(); _i++)
        {
            Add(_arrPairs[_i]);
        }
    }

    template<typename Key, typename Value>
    RedBlackTree<Key, Value>& RedBlackTree<Key, Value>::operator=(
        const RedBlackTree& nTree_)
    {
        if (this->m_pRoot == nTree_.m_pRoot)
        {
            return *this;
        }

        Reset();
        DynArray<Pair> _arrPairs = nTree_.GetArray();
        for (int _i = 0; _i < _arrPairs.GetSize(); _i++)
        {
            Add(_arrPairs[_i]);
        }

        return *this;
    }

    template<typename Key, typename Value>
    typename RedBlackTree<Key, Value>::Node* RedBlackTree<Key, Value>::LeftRotate(
        Node *pNode_)
    {
        assert(pNode_ != nullptr
                && pNode_->m_pRightChild != nullptr);

        Node *_pRight = pNode_->m_pRightChild;
        Node *_pLeft = pNode_->m_pLeftChild;
        Node *_pRightLeft = _pRight->m_pLeftChild;
        Node *_pRightRight = _pRight->m_pRightChild;
        Node *_pParent = pNode_->m_pParent;
        if (_pParent == nullptr)
        {
            m_pRoot = _pRight;
        }
        else if (_pParent->m_pLeftChild == pNode_)
        {
            _pParent->m_pLeftChild = _pRight;
        }
        else
        {
            _pParent->m_pRightChild = _pRight;
        }

        _pRight->m_pParent = _pParent;
        _pRight->m_pLeftChild = pNode_;
        _pRight->m_pRightChild = _pRightRight;

        pNode_->m_pParent = _pRight;
        pNode_->m_pLeftChild = _pLeft;
        pNode_->m_pRightChild = _pRightLeft;
        if (_pRightLeft != nullptr)
        {
            _pRightLeft->m_pParent = pNode_;
        }

        return _pRight;
    }

    template<typename Key, typename Value>
    typename RedBlackTree<Key, Value>::Node * RedBlackTree<Key, Value>::RightRotate(
        Node *pNode_)
    {
        assert(pNode_ != nullptr
                && pNode_->m_pLeftChild != nullptr);

        Node *_pLeft = pNode_->m_pLeftChild;
        Node *_pRight = pNode_->m_pRightChild;
        Node *_pLeftLeft = _pLeft->m_pLeftChild;
        Node *_pLeftRight = _pLeft->m_pRightChild;
        Node *_pParent = pNode_->m_pParent;
        if (_pParent == nullptr)
        {
            m_pRoot = _pLeft;
        }
        else if (_pParent->m_pLeftChild == pNode_)
        {
            _pParent->m_pLeftChild = _pLeft;
        }
        else
        {
            _pParent->m_pRightChild = _pLeft;
        }

        _pLeft->m_pParent = _pParent;
        _pLeft->m_pLeftChild = _pLeftLeft;
        _pLeft->m_pRightChild = pNode_;

        pNode_->m_pParent = _pLeft;
        pNode_->m_pLeftChild = _pLeftRight;
        pNode_->m_pRightChild = _pRight;
        if (_pLeftRight != nullptr)
        {
            _pLeftRight->m_pParent = pNode_;
        }

        return _pLeft;
    }

    template<typename Key, typename Value>
    bool RedBlackTree<Key, Value>::AdjustForParentColor(
        Node *pNode_)
    {
        assert(pNode_ != nullptr
                && pNode_->m_nColor == RED
                && pNode_->m_pParent != nullptr
                && pNode_->m_pParent->m_nColor == RED);

        Node *_pNode = pNode_;
        Node *_pParent = nullptr;
        Node *_pGrandFather = nullptr;
        Color _nColor = RED;
        while (true)
        {
            _pParent = nullptr;
            if (_pNode != nullptr)
            {
                _pParent = _pNode->m_pParent;
            }

            if (_pParent != nullptr)
            {
                _pGrandFather = _pParent->m_pParent;
            }

            if (_pNode == nullptr
                || _pParent == nullptr
                || _pParent->m_nColor == BLACK
                || _pGrandFather == nullptr)
            {
                break;
            }

            if (_pNode == _pParent->m_pLeftChild
                && _pParent == _pGrandFather->m_pLeftChild)
            {
                _pNode = RightRotate(_pGrandFather);
                _pNode->m_pLeftChild->m_nColor = (BLACK);
            }
            else if (_pNode == _pParent->m_pLeftChild
                && _pParent == _pGrandFather->m_pRightChild
                && _pGrandFather->m_pLeftChild == nullptr)
            {
                _pGrandFather = LeftRotate(_pGrandFather);
                LeftRotate(_pGrandFather->m_pLeftChild);
                _pGrandFather = RightRotate(_pGrandFather);
                _pGrandFather->m_nColor = BLACK;
                _pGrandFather->m_pLeftChild->m_nColor = RED;
                break;
            }
            else if (_pNode == _pParent->m_pLeftChild
                && _pParent == _pGrandFather->m_pRightChild
                && _pGrandFather->m_pLeftChild)
            {
                RightRotate(_pParent);
                _pNode = LeftRotate(_pGrandFather);
                _pNode->m_pRightChild->m_nColor = (BLACK);
            }
            else if (_pNode == _pParent->m_pRightChild
                && _pParent == _pGrandFather->m_pLeftChild
                && _pGrandFather->m_pRightChild == nullptr)
            {
                LeftRotate(_pParent);
                _pNode = RightRotate(_pGrandFather);
                _pNode->m_nColor = BLACK;
                _pNode->m_pRightChild->m_nColor = RED;
                break;
            }
            else if (_pNode == _pParent->m_pRightChild
                && _pParent == _pGrandFather->m_pLeftChild
                && _pGrandFather->m_pRightChild)
            {
                LeftRotate(_pParent);
                _pNode = RightRotate(_pGrandFather);
                _pNode->m_pLeftChild->m_nColor = (BLACK);
            }
            else if (_pNode == _pParent->m_pRightChild
                && _pGrandFather->m_pRightChild == _pParent)
            {
                _pNode = LeftRotate(_pGrandFather);
                _pNode->m_pRightChild->m_nColor = (BLACK);
            }
            else
            {
                assert(false);
            }
        }

        m_pRoot->m_nColor = BLACK;
        return true;
    }

    template<typename Key, typename Value>
    void RedBlackTree<Key, Value>::AdjustForBlackBlack(
        Node *pNode_)
    {
        assert(pNode_ != nullptr
                && pNode_->m_nColor == BLACKBLACK);

        Node *_pParent = nullptr;
        while (true)
        {
            _pParent = nullptr;
            if (pNode_)
            {
                _pParent = pNode_->m_pParent;
            }

            if (pNode_ == nullptr
                || _pParent == nullptr
                || pNode_->m_nColor != BLACKBLACK)
            {
                break;
            }

            if (pNode_ == _pParent->m_pLeftChild
                && _pParent->m_nColor == BLACK
                && _pParent->m_pRightChild->m_nColor == BLACK)
            {
                if (_pParent->m_pRightChild->m_pLeftChild->m_nColor == BLACK)
                {
                    _pParent = LeftRotate(_pParent);
                    _pParent->m_nColor = BLACKBLACK;
                    _pParent->m_pLeftChild->m_pLeftChild->m_nColor = BLACK;
                    _pParent->m_pLeftChild->m_nColor = RED;
                    pNode_ = _pParent;
                }
                else if (_pParent->m_pRightChild->m_pLeftChild->m_nColor == RED
                    && _pParent->m_pRightChild->m_pRightChild->m_nColor == RED)
                {
                    _pParent = LeftRotate(_pParent);
                    LeftRotate(_pParent->m_pLeftChild);
                    _pParent->m_pLeftChild->m_pLeftChild->m_pLeftChild->m_nColor = BLACK;
                    _pParent->m_pLeftChild->m_pLeftChild->m_nColor = RED;
                    _pParent->m_pLeftChild->m_nColor = BLACK;
                    _pParent->m_pRightChild->m_nColor = BLACK;
                    break;
                }
                else if (_pParent->m_pRightChild->m_pLeftChild->m_nColor == RED
                    && _pParent->m_pRightChild->m_pRightChild->m_nColor == BLACK)
                {
                    RightRotate(_pParent->m_pRightChild);
                    _pParent = LeftRotate(_pParent);
                    _pParent->m_pLeftChild->m_pLeftChild->m_nColor = BLACK;
                    _pParent->m_nColor = BLACK;
                    break;
                }
                else
                {
                    assert(false);
                    throw "unexpected action";
                }
            }
            else if (pNode_ == _pParent->m_pLeftChild
                && _pParent->m_nColor == BLACK
                && _pParent->m_pRightChild->m_nColor == RED)
            {
                if (_pParent->m_pRightChild->m_pLeftChild->m_pLeftChild->m_nColor == BLACK)
                {
                    _pParent = LeftRotate(_pParent);
                    _pParent->m_nColor = BLACK;
                    LeftRotate(_pParent->m_pLeftChild);
                    _pParent->m_pLeftChild->m_pLeftChild->m_pLeftChild->m_nColor = BLACK;
                    _pParent->m_pLeftChild->m_pLeftChild->m_nColor = RED;
                    break;
                }
                else
                {
                    _pParent = LeftRotate(_pParent);
                    RightRotate(_pParent->m_pLeftChild->m_pRightChild);
                    LeftRotate(_pParent->m_pLeftChild);
                    _pParent->m_nColor = BLACK;
                    _pParent->m_pLeftChild->m_pLeftChild->m_pLeftChild->m_nColor = BLACK;
                    break;
                }
            }
            else if (pNode_ == _pParent->m_pLeftChild
                && _pParent->m_nColor == RED)
            {
                _pParent = LeftRotate(_pParent);
                _pParent->m_nColor = BLACK;
                _pParent->m_pLeftChild->m_nColor = RED;
                _pParent->m_pLeftChild->m_pLeftChild->m_nColor = BLACK;
                if (_pParent->m_pLeftChild->m_pRightChild->m_nColor == RED)
                {
                    AdjustForParentColor(_pParent->m_pLeftChild->m_pRightChild);
                }

                break;
            }
            else if (pNode_ == _pParent->m_pRightChild
                && _pParent->m_nColor == BLACK
                && _pParent->m_pLeftChild->m_nColor == BLACK)
            {
                if (_pParent->m_pLeftChild->m_pRightChild->m_nColor == BLACK)
                {
                    _pParent = RightRotate(_pParent);
                    _pParent->m_pRightChild->m_pRightChild->m_nColor = BLACK;
                    _pParent->m_pRightChild->m_nColor = RED;
                    _pParent->m_nColor = BLACKBLACK;
                    pNode_ = _pParent;
                }
                else if (_pParent->m_pLeftChild->m_pRightChild->m_nColor == RED)
                {
                    LeftRotate(_pParent->m_pLeftChild);
                    _pParent = RightRotate(_pParent);
                    _pParent->m_nColor = BLACK;
                    _pParent->m_pRightChild->m_pRightChild->m_nColor = BLACK;
                    break;
                }
                else
                {
                    assert(false);
                    throw "unexpected situation";
                }
            }
            else if (pNode_ == _pParent->m_pRightChild
                && _pParent->m_nColor == BLACK
                && _pParent->m_pLeftChild->m_nColor == RED)
            {
                if (_pParent->m_pLeftChild->m_pRightChild->m_pRightChild->m_nColor == BLACK)
                {
                    _pParent = RightRotate(_pParent);
                    RightRotate(_pParent->m_pRightChild);
                    _pParent->m_pRightChild->m_pRightChild->m_pRightChild->m_nColor = BLACK;
                    _pParent->m_pRightChild->m_pRightChild->m_nColor = RED;
                    _pParent->m_nColor = BLACK;
                    break;
                }
                else
                {
                    _pParent = RightRotate(_pParent);
                    LeftRotate(_pParent->m_pRightChild->m_pLeftChild);
                    RightRotate(_pParent->m_pRightChild);
                    _pParent->m_nColor = BLACK;
                    _pParent->m_pRightChild->m_pRightChild->m_pRightChild->m_nColor = BLACK;
                    break;
                }
            }
            else if (pNode_ == _pParent->m_pRightChild
                && _pParent->m_nColor == RED)
            {
                _pParent = RightRotate(_pParent);
                _pParent->m_pRightChild->m_pRightChild->m_nColor = BLACK;
                if (_pParent->m_pRightChild->m_pLeftChild->m_nColor == RED)
                {
                    AdjustForParentColor(_pParent->m_pRightChild->m_pLeftChild);
                }

                break;
            }
            else
            {
                assert(false);
                throw "unexpected situation";
            }
        }

        m_pRoot->m_nColor = BLACK;
        assert(CheckValid());
    }

    template<typename Key, typename Value>
    void RedBlackTree<Key, Value>::AdjustForBlackHeight(
        Node *pNode_)
    {
        assert(pNode_ != nullptr);
        if (pNode_->m_pLeftChild == nullptr
            && pNode_->m_pRightChild != nullptr)
        {
            if (pNode_->m_nColor == RED)
            {
                Node *_pTemp = LeftRotate(pNode_);
                if (_pTemp->m_pLeftChild->m_pRightChild != nullptr)
                {
                    AdjustForParentColor(_pTemp->m_pLeftChild->m_pRightChild);
                }

                assert(CheckValid());
            }
            else if (pNode_->m_nColor == BLACK)
            {
                if (pNode_->m_pRightChild->m_nColor == RED)
                {
                    Node *_pTemp = LeftRotate(pNode_);
                    LeftRotate(_pTemp->m_pLeftChild);
                    _pTemp->m_nColor = BLACK;
                    _pTemp->m_pLeftChild->m_pLeftChild->m_nColor = RED;
                    if (_pTemp->m_pLeftChild->m_pLeftChild->m_pRightChild != nullptr)
                    {
                        assert(_pTemp->m_pLeftChild->m_pLeftChild->m_pRightChild->m_nColor == RED);
                        AdjustForParentColor(_pTemp->m_pLeftChild->m_pLeftChild->m_pRightChild);
                        assert(CheckValid());
                    }
                    else
                    {
                        assert(CheckValid());
                    }
                }
                else if (pNode_->m_pRightChild->m_nColor == BLACK)
                {
                    Node *_pTemp = LeftRotate(pNode_);
                    if (_pTemp->m_pRightChild != nullptr)
                    {
                        assert(_pTemp->m_pRightChild->m_nColor == RED);
                        _pTemp->m_pRightChild->m_nColor = BLACK;
                        assert(CheckValid());
                    }
                    else
                    {
                        if (_pTemp->m_pLeftChild->m_pRightChild != nullptr)
                        {
                            assert(_pTemp->m_pLeftChild->m_pRightChild->m_nColor == RED);
                            LeftRotate(_pTemp->m_pLeftChild);
                            _pTemp = RightRotate(_pTemp);
                            _pTemp->m_nColor = BLACK;
                            assert(_pTemp->m_pLeftChild->m_nColor == BLACK);
                            assert(_pTemp->m_pRightChild->m_nColor == BLACK);
                            assert(CheckValid());
                        }
                        else
                        {
                            _pTemp->m_pLeftChild->m_nColor = RED;
                            _pTemp->m_nColor = BLACKBLACK;
                            AdjustForBlackBlack(_pTemp);
                            assert(CheckValid());
                        }
                    }
                }
                else
                {
                    assert(false);
                    throw "The right child of the adjust node not has a right color";
                }
            }
            else
            {
                assert(false);
                throw "The adjust node not has a right color";
            }
        }
        else if (pNode_->m_pLeftChild != nullptr
            && pNode_->m_pRightChild == nullptr)
        {
            if (pNode_->m_nColor == RED)
            {
                Node *_pTemp = RightRotate(pNode_);
                if (_pTemp->m_pRightChild->m_pLeftChild != nullptr)
                {
                    AdjustForParentColor(_pTemp->m_pRightChild->m_pLeftChild);
                }

                assert(CheckValid());
            }
            else if (pNode_->m_nColor == BLACK)
            {
                if (pNode_->m_pLeftChild->m_nColor == RED)
                {
                    Node *_pTemp = RightRotate(pNode_);
                    RightRotate(_pTemp->m_pRightChild);
                    _pTemp->m_nColor = BLACK;
                    _pTemp->m_pRightChild->m_pRightChild->m_nColor = RED;
                    if (_pTemp->m_pRightChild->m_pRightChild->m_pLeftChild != nullptr)
                    {
                        AdjustForParentColor(_pTemp->m_pRightChild->m_pRightChild->m_pLeftChild);
                        assert(CheckValid());
                    }
                    else
                    {
                        assert(CheckValid());
                    }
                }
                else if (pNode_->m_pLeftChild->m_nColor == BLACK)
                {
                    Node *_pTemp = RightRotate(pNode_);
                    if (_pTemp->m_pLeftChild)
                    {
                        assert(_pTemp->m_pLeftChild->m_nColor == RED);
                        _pTemp->m_pLeftChild->m_nColor = BLACK;
                        assert(CheckValid());
                    }
                    else
                    {
                        if (_pTemp->m_pRightChild->m_pLeftChild)
                        {
                            assert(_pTemp->m_pRightChild->m_pLeftChild->m_nColor == RED);
                            RightRotate(_pTemp->m_pRightChild);
                            _pTemp = LeftRotate(_pTemp);
                            _pTemp->m_nColor = BLACK;
                            assert(_pTemp->m_pLeftChild->m_nColor == BLACK);
                            assert(_pTemp->m_pRightChild->m_nColor == BLACK);
                            assert(CheckValid());
                        }
                        else
                        {
                            _pTemp->m_pRightChild->m_nColor = RED;
                            _pTemp->m_nColor = BLACKBLACK;
                            AdjustForBlackBlack(_pTemp);
                            assert(CheckValid());
                        }
                    }
                }
                else
                {
                    assert(false);
                    throw "The left child of the adjust node not has a right color";
                }
            }
            else
            {
                assert(false);
                throw "The adjust node not has a right color";
            }
        }
        else
        {
            assert(false);
            throw "The adjust node has no child";
        }

        assert(CheckValid());
    }

    template<typename Key, typename Value>
    bool RedBlackTree<Key, Value>::CheckValid() const
    {
        bool _bValid = true;
        if (m_pRoot == nullptr)
        {
            return true;
        }

        // 根黑
        if (m_pRoot
            && m_pRoot->m_nColor != BLACK)
        {
            assert(false);
            _bValid = false;
            return false;
        }

        // 父红，则子黑
        // 颜色须为红或黑
        // 黑高一致
        PreVisit([&_bValid, this](Node* pNode_)->void
        {
            if (pNode_ == nullptr
                || _bValid == false)
            {
                return;
            }

            if (pNode_->m_nColor != RED
                && pNode_->m_nColor != BLACK)
            {
                _bValid = false;
                return;
            }

            if (pNode_->m_nColor == RED)
            {
                if (pNode_->m_pLeftChild != nullptr
                    && pNode_->m_pLeftChild->m_nColor != BLACK)
                {
                    _bValid = false;
                    return;
                }

                if (pNode_->m_pRightChild != nullptr
                    && pNode_->m_pRightChild->m_nColor != BLACK)
                {
                    _bValid = false;
                    return;
                }
            }

            int _nLeftChildTreeBlackHeight = this->GetBlackHeight(pNode_->m_pLeftChild);
            int _nRightChildTreeBlackHeight = this->GetBlackHeight(pNode_->m_pRightChild);
            if (_nLeftChildTreeBlackHeight != _nRightChildTreeBlackHeight
                || _nLeftChildTreeBlackHeight == -1)
            {
                _bValid = false;
                return;
            }
        }, m_pRoot);

        return _bValid;
    }

    template<typename Key, typename Value>
    int RedBlackTree<Key, Value>::GetBlackHeight(Node *pNode_) const
    {
        int _nBlackHeight = 0;
        if (pNode_ == nullptr)
        {
            return _nBlackHeight;
        }

        if (pNode_->m_nColor == BLACK)
        {
            _nBlackHeight = 1;
        }

        int _nLeft = GetBlackHeight(pNode_->m_pLeftChild);
        int _nRight = GetBlackHeight(pNode_->m_pRightChild);
        if (_nLeft != _nRight
            || _nLeft == -1
            || _nRight == -1)
        {
            return -1;
        }
        else
        {
            return _nLeft + _nBlackHeight;
        }
    }

}

#endif

