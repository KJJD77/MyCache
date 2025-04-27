#pragma once
namespace MyCache
{
    template <typename Key, typename Value>
    class CachePolicy
    {
        // 公开接口
    public:
        virtual ~CachePolicy() {};
        // 放入元素
        virtual void put(Key key, Value value) = 0;

        // 通过引用返回val值，bool表示寻找情况
        virtual bool get(Key key, Value &value) = 0;

        // 只返回值
        virtual Value get(Key key) = 0;
    };
}
