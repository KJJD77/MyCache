#pragma once

namespace MyCache
{
    template <typename Key, typename Value>
    class ArcCacheNode
    {
    private:
        Key key_;
        Value value_;
        size_t accessCount_; // 访问次数
        std::weak_ptr<ArcCacheNode> pre_; // 前驱节点
        std::shared_ptr<ArcCacheNode> next_; // 后继节点
    public:
        ArcCacheNode():accessCount_(1), next_(nullptr) {}
        ArcCacheNode(Key key, Value value) : key_(key), value_(value), accessCount_(1), next_(nullptr) {}
        // Getters
        Key getKey() const { return key_; }
        Value getValue() const { return value_; }
        size_t getAccessCount() const { return accessCount_; }
        //setters
        void setKey(Key key) { key_ = key; }
        void setValue(Value value) { value_ = value; }
        void incrementAccessCount() { accessCount_++; }
        template<typename K,typename V>
        friend class ArcLruPart; // 允许ArcCache访问私有成员
        template<typename K,typename V>
        friend class ArcLfuPart; // 允许ArcCache访问私有成员
    };
}