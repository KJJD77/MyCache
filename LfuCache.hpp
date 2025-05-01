#pragma once

#include "CachePolicy.h"
#include <mutex>
#include <thread>
#include <unordered_map>
#include <cmath>

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
    class LfuCache : public CachePolicy<Key, Value>
    {
    private:
        using Node = typename FreqList<Key, Value>::Node;
        using NodePtr = std::shared_ptr<Node>;
        using NodeMap = std::unordered_map<Key, NodePtr>;

        std::unordered_map<int, FreqList<Key, Value> *> freqToFreqList_;
        NodeMap nodeMap_;   // 全局找Node
        int capacity_;      // 容量
        int minFreq_;       // 最小访问次数(O1存取的关键)
        int curAverageNum_; // 当前访问次数平均值
        int maxAverageNum_; // 最大容忍访问次数平均值
        int curTotalNum_;   // 当前总访问次数
        std::mutex mutex_;  // 互斥锁

    public:
        ~LfuCache() override = default;

        LfuCache(int capacity, int maxAverageNum = 1000000) : capacity_(capacity), maxAverageNum_(maxAverageNum), curAverageNum_(0), curTotalNum_(0), minFreq_(INT8_MAX)
        {
        }

        void put(Key key, Value value) override
        {
            if (capacity_ == 0)
                return;
            std::lock_guard<std::mutex> lock(mutex_);

            if (nodeMap_.find(key) != nodeMap_.end())
            {
                NodePtr node = nodeMap_.find(key)->second;
                node->value = value;
                getInternal(node, value);
                return;
            }

            putInternal(key, value);
        }

        bool get(Key key, Value &value) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it=nodeMap_.find(key);
            if(it!=nodeMap_.end())
            {
                getInternal(it->second,value);
                return true;
            }
            return false;
        }

        Value get(Key key) override
        {
            Value value{};
            get(key, value);
            return value;
        }

        // 清空
        void purge()
        {
            nodeMap_.clear();
            freqToFreqList_.clear();
        }

    private:
        void getInternal(NodePtr node, Value &value); // 获取缓存，并且freq+1
        void putInternal(Key key, Value value);       // 放置缓存，并且freq+1

        void kickOut(); // 移除第一个

        void removeFromFreqList(NodePtr node); // 从频率列表中移除节点
        void addToFreqList(NodePtr node);      // 添加到频率列表

        void addFreqNum();              // 总访问频次++
        void decreaseFreqNum(int num);  // 减少总访问频次
        void handleOverMaxAverageNum(); // 解决平均频率太高
        void updateMinFreq();           // 更新最小的频率
    };

    template <typename Key, typename Value>
    void LfuCache<Key, Value>::getInternal(NodePtr node, Value &value)
    {
        // 找到之后需要将其从低访问频次的链表中删除，并且添加到+1的访问频次链表中，
        // 访问频次+1, 然后把value值返回
        value = node->value;
        // 从原来的链表中删除
        removeFromFreqList(node);
        node->freq++;
        addToFreqList(node);
        // 如果当前node的访问频次如果等于minFreq+1，并且其前驱链表为空，则说明
        // freqToFreqList_[node->freq - 1]链表因node的迁移已经空了，需要更新最小访问频次
        if (node->freq - 1 == minFreq_ && freqToFreqList_[minFreq_]->isEmpty())
            minFreq_++;
        // 总访问频次和当前平均访问频次都随之增加
        addFreqNum();
    }

    template <typename Key, typename Value>
    void LfuCache<Key, Value>::putInternal(Key key, Value value)
    {
        // 如果不在缓存中，则需要判断缓存是否已满
        if (nodeMap_.size() == capacity_)
        {
            // 缓存已满，删除最不常访问的结点，更新当前平均访问频次和总访问频次
            kickOut();
        }
        NodePtr node = std::make_shared<Node>(key, value);
        addToFreqList(node);
        nodeMap_[key] = node;
        addFreqNum();
        minFreq_ = std::min(minFreq_, 1);
    }

    template <typename Key, typename Value>
    void LfuCache<Key, Value>::removeFromFreqList(NodePtr node)
    {
        if (!node)
            return;
        auto freq = node->freq;
        freqToFreqList_[freq]->removeNode(node);
    }

    template <typename Key, typename Value>
    void LfuCache<Key, Value>::addToFreqList(NodePtr node)
    {
        if (!node)
            return;
        auto freq = node->freq;
        if (freqToFreqList_.find(freq) == freqToFreqList_.end()) // 不存在
        {
            freqToFreqList_[freq] = new FreqList<Key, Value>(freq);
        }
        freqToFreqList_[freq]->addNode(node);
    }

    template <typename Key, typename Value>
    void LfuCache<Key, Value>::kickOut()
    {
        NodePtr node = freqToFreqList_[minFreq_]->getFirstNode();
        removeFromFreqList(node);
        nodeMap_.erase(node->key);
        decreaseFreqNum(node->freq);
    }
    template <typename Key, typename Value>
    void LfuCache<Key, Value>::addFreqNum()
    {
        curTotalNum_++;
        if (nodeMap_.empty())
            curAverageNum_ = 0;
        else
            curAverageNum_ = std::ceil(curTotalNum_ / nodeMap_.size());
        if (curAverageNum_ > maxAverageNum_)
            handleOverMaxAverageNum();
    }
    template <typename Key, typename Value>
    void LfuCache<Key, Value>::decreaseFreqNum(int num)
    {
        curTotalNum_ -= num;
        if (nodeMap_.empty())
            curAverageNum_ = 0;
        else
            curAverageNum_ = curTotalNum_ / nodeMap_.size();
    }
    template <typename Key, typename Value>
    void LfuCache<Key, Value>::handleOverMaxAverageNum()
    {
        if (nodeMap_.size() == 0)
            return;
        for (auto it = nodeMap_.begin(); it != nodeMap_.end(); it++)
        {
            if (!it->second)
                continue;
            NodePtr node = it->second;

            // 先去除node
            removeFromFreqList(node);

            // 减去freq
            node->freq -= maxAverageNum_ / 2;
            if (node->freq < 1)
                node->freq = 1;
            // 再加回去
            addToFreqList(node);
        }
        updateMinFreq();
    }

    template <typename Key, typename Value>
    void LfuCache<Key, Value>::updateMinFreq()
    {
        minFreq_ = INT8_MAX;
        for (const auto &pair : freqToFreqList_)
        {
            if (pair.second && !pair.second->isEmpty())
                minFreq_ = std::min(minFreq_, pair.first);
        }
        if (minFreq_ == INT8_MAX)
            minFreq_ = 1;
    }

}
