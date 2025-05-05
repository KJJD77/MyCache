#pragma once
#include <memory>
#include <map>
#include "../CachePolicy.h"
#include "ArcLfuPart.hpp"
#include "ArcLruPart.hpp"

namespace MyCache
{
    template <typename Key, typename Value>
    class ArcCache : public CachePolicy<Key, Value>
    {
    public:
        ArcCache(size_t capacity, size_t transformThreshold=2)
            : capacity_(capacity), transformThreshold_(transformThreshold)
        {
            lruPart_ = std::make_unique<ArcLruPart<Key, Value>>(capacity, transformThreshold);
            lfuPart_ = std::make_unique<ArcLfuPart<Key, Value>>(capacity, transformThreshold);
        }

        ~ArcCache() override = default;

        void put(Key key, Value value) override
        {
            bool inGhost = checkGhostCaches(key);

            if (!inGhost)
            {
                if (lruPart_->put(key, value))
                {
                    lfuPart_->put(key, value);
                }
            }
            else
            {
                lruPart_->put(key, value);
            }
        }

        bool get(Key key, Value &value) override
        {
            checkGhostCaches(key);

            bool shouldTransform = false;
            if (lruPart_->get(key, value, shouldTransform))
            {
                if (shouldTransform)
                {
                    lfuPart_->put(key, value);
                }
                return true;
            }
            return lfuPart_->get(key, value);
        }

        Value get(Key key) override
        {
            Value value{};
            get(key, value);
            return value;
        }

    private:
        bool checkGhostCaches(Key key)
        {
            bool inGhost = false;
            if (lruPart_->checkGhost(key))
            {
                if (lfuPart_->decreaseCapacity())
                {
                    lruPart_->increaseCapacity();
                }
                inGhost = true;
            }
            else if (lfuPart_->checkGhost(key))
            {
                if (lruPart_->decreaseCapacity())
                {
                    lfuPart_->increaseCapacity();
                }
                inGhost = true;
            }
            return inGhost;
        }

        std::unique_ptr<ArcLruPart<Key, Value>> lruPart_;
        std::unique_ptr<ArcLfuPart<Key, Value>> lfuPart_;

        size_t capacity_;           // 总容量
        size_t transformThreshold_; // 转换阈值
    };
} // namespace MyCache
