#pragma once
#include <mutex>
#include <unordered_map>
#include "ArcCacheNode.hpp"

namespace MyCache
{
    template <typename Key, typename Value>
    class ArcLruPart
    {
    public:
        using NodeType = ArcCacheNode<Key, Value>;
        using NodePtr = std::shared_ptr<NodeType>;
        using NodeMap = std::unordered_map<Key, NodePtr>;

        explicit ArcLruPart(size_t capacity, size_t transformThresholdm) : capacity_(capacity), ghostCapacity_(capacity), transformThreshold_(transformThresholdm)
        {
            initList();
        }

        bool put(Key key, Value value)
        {
            if (capacity_ <= 0)
            {
                return false;
            }
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = mainCache_.find(key);
            if (it != mainCache_.end())
            {
                // 如果主链表中存在该节点，更新值并移动到头部
                return updateExistingNode(it->second, value);
            }
            // 如果主链表中不存在该节点，创建新节点
            return addNeWNode(key, value);
        }

        bool get(Key key, Value &value, bool &shouldTransform)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = mainCache_.find(key);
            if (it != mainCache_.end())
            {
                // 如果主链表中存在该节点，更新值并移动到头部
                shouldTransform = updateNodeAccess(it->second);
                value = it->second->getValue();
                return true;
            }
            return false;
        }
        bool checkGhost(Key key)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = ghostCache_.find(key);
            if (it != ghostCache_.end())
            {
                removeFromGhost(it->second);
                ghostCache_.erase(it);
                return true;
            }
            return false;
        }

        void increaseCapacity() { ++capacity_; }

        bool decreaseCapacity()
        {
            if (capacity_ <= 0)
                return false;
            if (mainCache_.size() >= capacity_)
            {
                evictLeastRecent();
            }
            capacity_--;
            return true;
        }

    private:
        size_t capacity_;           // 容量
        size_t ghostCapacity_;      // 副链表容量
        size_t transformThreshold_; // 转换门槛值

        NodePtr mainHead_;  // 主链表头
        NodePtr mainTail_;  // 主链表尾
        NodePtr ghostHead_; // 淘汰链表头
        NodePtr ghostTail_; // 淘汰表尾

        std::mutex mutex_;

        NodeMap mainCache_;  // key -> mainNodePtr 映射
        NodeMap ghostCache_; // key -> ghostNodePtr 映射

        void initList()
        {
            mainHead_ = std::make_shared<NodeType>();
            mainTail_ = std::make_shared<NodeType>();
            ghostHead_ = std::make_shared<NodeType>();
            ghostTail_ = std::make_shared<NodeType>();

            mainHead_->next_ = mainTail_;
            mainTail_->pre_ = mainHead_;

            ghostHead_->next_ = ghostTail_;
            ghostTail_->pre_ = ghostHead_;
        }

        bool updateExistingNode(NodePtr node, Value &value)
        {
            node->setValue(value);
            moveToFront(node);
            return true;
        }

        void moveToFront(NodePtr node)
        {
            // 从主链表中移除节点
            removeFromMain(node);

            // 将节点添加到主链表头部
            mainHead_->next_->pre_ = node;
            node->next_ = mainHead_->next_;
            mainHead_->next_ = node;
            node->pre_ = mainHead_;
        }

        bool addNeWNode(Key key, Value value)
        {
            if (mainCache_.size() >= capacity_)
            {
                // 如果主链表已满，淘汰最少使用的节点
                evictLeastRecent();
            }
            NodePtr newNode = std::make_shared<NodeType>(key, value);
            mainCache_[key] = newNode;
            addToFront(newNode);
            return true;
        }

        void addToFront(NodePtr node)
        {
            // 将节点移动到主链表头部
            mainHead_->next_->pre_ = node;
            node->next_ = mainHead_->next_;
            mainHead_->next_ = node;
            node->pre_ = mainHead_;
        }

        void evictLeastRecent()
        {
            // 淘汰最少使用的节点
            NodePtr leastRecent = mainTail_->pre_.lock();
            if (leastRecent == mainHead_ || !leastRecent)
                return; // 主链表为空

            removeFromMain(leastRecent);

            if (ghostCache_.size() >= ghostCapacity_)
            {
                // 如果副链表已满，淘汰最少使用的节点
                removeOldestFromGhost();
            }
            addGhost(leastRecent);
            mainCache_.erase(leastRecent->getKey());
        }

        void removeByKey(Key key)
        {
            // 从主链表中移除节点
            auto it = mainCache_.find(key);
            if (it != mainCache_.end())
            {
                NodePtr node = it->second;
                removeFromMain(node);
                mainCache_.erase(it);
            }
        }

        void removeFromMain(NodePtr node)
        {
            // 从主链表中移除节点
            if (!node->pre_.expired() && node->next_)
            {
                NodePtr preNode = node->pre_.lock();
                preNode->next_ = node->next_;
                node->next_->pre_ = preNode;
                node->next_ = nullptr;
            }
        }
        void removeOldestFromGhost()
        {
            // 淘汰幽灵链表中最少使用的节点
            NodePtr oldest = ghostTail_->pre_.lock();
            if (oldest == ghostHead_ || !oldest)
                return; // 幽灵链表为空
            // 从幽灵链表中移除节点
            removeFromGhost(oldest);
            ghostCache_.erase(oldest->getKey());
        }
        void removeFromGhost(NodePtr node)
        {
            // 从幽灵链表中移除节点
            if (!node->pre_.expired() && node->next_)
            {
                NodePtr preNode = node->pre_.lock();
                preNode->next_ = node->next_;
                node->next_->pre_ = preNode;
                node->next_ = nullptr;
            }
        }
        void addGhost(NodePtr node)
        {
            // 将节点添加到幽灵链表
            node->accessCount_ = 1;
            // 添加到幽灵缓存映射
            ghostCache_[node->getKey()] = node;

            ghostHead_->next_->pre_ = node;
            node->next_ = ghostHead_->next_;
            ghostHead_->next_ = node;
            node->pre_ = ghostHead_;
        }

        bool updateNodeAccess(NodePtr node)
        {
            // 更新访问次数
            moveToFront(node);
            node->incrementAccessCount();
            return node->getAccessCount() >= transformThreshold_;
        }
    };
}