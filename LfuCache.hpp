#pragma once

#include "CachePolicy.h"
#include <mutex>
#include <thread>
#include <unordered_map>

namespace MyCache
{
    template <typename Key, typename Value>
    class LfuCache;

    template <typename Key, typename Value>
    class FreqList
    {
    private:
        struct Node
        {
            int freq; // 访问频次
            Key key;
            Value value;
            std::weak_ptr<Node> pre; // 打破循环依赖
            std::shared_ptr<Node> next;

            Node() : freq(1), next(nullptr) {}
            Node(Key key, Value value) : freq(1), key(key), value(value), next(nullptr) {}
        };
        using NodePtr = std::shared_ptr<Node>;
        std::shared_ptr<Node> head_;
        std::shared_ptr<Node> tail_;
        int freq_; // 当前链表的频率是多少

    public:
        explicit FreqList(int freq) : freq_(freq)
        {
            head_ = std::make_shared<Node>();
            tail_ = std::make_shared<Node>();
            head_->next = tail_;
            tail_->pre = head_;
        }

        bool isEmpty() const
        {
            return head_->next == tail_;
        }

        void addNode(NodePtr node)
        {
            if (!node || !head_ || !tail_)
                return;
            node->next = tail_;
            auto pre = tail_->pre.lock();
            node->pre = pre;
            pre->next = node;
            tail_->pre = node;
        }

        void removeNode(NodePtr node)
        {
            if (!node || !head_ || !tail_)
                return;
            if (!node->next || node->pre.expired())
                return;
            auto pre = node->pre.lock();
            pre->next = node->next;
            node->next->pre = pre;
            node->next = nullptr;
        }

        NodePtr getFirstNode()
        {
            return head_->next;
        }
        friend class LfuCache<Key, Value>;
    };
    template <typename Key, typename Value>
    class LfuCache: public 
    {
        private:

    };
}
