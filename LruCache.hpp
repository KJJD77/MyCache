#pragma once

#include "CachePolicy.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <condition_variable>
namespace MyCache
{
    template <typename Key, typename Value>
    class LruCache;
    template <typename Key, typename Value>
    class LruNode
    {
    public:
        std::condition_variable cd;
        LruNode(Key key_, Value value_) : key_(key_), value_(value_), accessCount_(1) {};
        Key getKey() const
        {
            return this->key_;
        }
        Value getValue() const
        {
            return this->value_;
        }
        size_t getAccessCount() const
        {
            return this->accessCount_;
        }
        void setValue(Value value_)
        {
            this->value_ = value_;
        }
        // 返回递增后的值
        int increasementAccessCount()
        {
            return ++this->accessCount_;
        }

    private:
        std::weak_ptr<LruNode<Key, Value>> prev_; // 打破循环引用
        std::shared_ptr<LruNode<Key, Value>> next_;
        Key key_;
        Value value_;
        size_t accessCount_; // 访问次数
        friend class LruCache<Key, Value>;
    };

    template <typename Key, typename Value>
    class LruCache : public CachePolicy<Key, Value>
    {
    public:
        using LruNodeType = LruNode<Key, Value>;
        using NodePtr = std::shared_ptr<LruNodeType>;
        using NodeMap = std::unordered_map<Key, NodePtr>;

        ~LruCache() = default;

        LruCache(int capacity_) : capacity_(capacity_)
        {
            init();
        }

        void put(Key key, Value value) override
        {
            if (this->capacity_ <= 0)
                return;
                //上锁
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = nodeMap_.find(key);
            if (it != nodeMap_.end())
            {
                updateExistingNode(it->second, value);
                return;
            }
            addNewNode(key, value);
        }

        bool get(Key key, Value &value) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it=nodeMap_.find(key);
            if(it==nodeMap_.end())
                return false;
            it->second->increasementAccessCount();
            // std::cerr<<it->second->getAccessCount();
            moveToMostRecent(it->second);
            value=it->second->getValue();
            return true;
        }

        Value get(Key key) override
        {
            Value value{};
            get(key,value);
            return value;
        }

        void remove(Key key)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it=nodeMap_.find(key);
            if(it==nodeMap_.end())
                return;
            removeNode(it->second);
            nodeMap_.erase(it);    
        }

    private:
        void init()
        {
            dummyHead_ = std::make_shared<LruNodeType>(Key(), Value());
            dummyTail_ = std::make_shared<LruNodeType>(Key(), Value());
            dummyHead_->next_ = dummyTail_;
            dummyTail_->prev_ = dummyHead_;
        }
        void updateExistingNode(NodePtr node, const Value &value)
        {
            node->setValue(value);
            moveToMostRecent(node);
        }
        void removeNode(NodePtr node)
        {
            if (!node->prev_.expired() && node->next_)
            {
                auto prev_ = (node->prev_).lock();
                auto next_ = node->next_;
                prev_->next_ = next_;
                next_->prev_ = prev_;
                node->next_ = nullptr;
            }
        }
        void moveToMostRecent(NodePtr node)
        {
            removeNode(node);
            insertNode(node);
        }
        void insertNode(NodePtr node)
        {
            node->next_=dummyTail_;
            node->prev_=dummyTail_->prev_;
            dummyTail_->prev_.lock()->next_=node;
            dummyTail_->prev_ = node;
        }
        void addNewNode(const Key &key, const Value &value)
        {
            if(nodeMap_.size()>=capacity_)
                evictLeastRecent();
            NodePtr newNode=std::make_shared<LruNodeType>(key,value);
            insertNode(newNode);
            nodeMap_[key] = newNode;
        }
        //驱逐链表表头
        void evictLeastRecent()
        {
            NodePtr leastRecent =  dummyHead_->next_;
            removeNode(leastRecent);
            nodeMap_.erase(leastRecent->getKey());
        }
        int capacity_;
        NodeMap nodeMap_;
        std::mutex mutex_;
        // 虚拟头节点
        NodePtr dummyHead_;
        // 虚拟尾节点
        NodePtr dummyTail_;
    };
}
