#include "LruCache.hpp"
#include "LfuCache.hpp"
#include "ArcCache/ArcCache.hpp"
#include "CachePolicy.h"

#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>

// 辅助函数：打印结果
void printResults(const std::string &testName, int capacity,
                  const std::vector<int> &get_operations,
                  const std::vector<int> &hits)
{
    std::cout << "=== " << testName << " 结果汇总 ===" << std::endl;
    std::cout << "缓存大小: " << capacity << std::endl;

    // 假设对应的算法名称已在测试函数中定义
    std::vector<std::string> names;
    if (hits.size() == 3)
    {
        names = {"LRU", "LFU", "ARC"};
    }
    else if (hits.size() == 4)
    {
        names = {"LRU", "LFU", "ARC", "LRU-K"};
    }
    else if (hits.size() == 5)
    {
        names = {"LRU", "LFU", "ARC", "LRU-K", "LFU-Aging"};
    }

    for (size_t i = 0; i < hits.size(); ++i)
    {
        double hitRate = 100.0 * hits[i] / get_operations[i];
        std::cout << (i < names.size() ? names[i] : "Algorithm " + std::to_string(i + 1))
                  << " - 命中率: " << std::fixed << std::setprecision(2)
                  << hitRate << "% ";
        // 添加具体命中次数和总操作次数
        std::cout << "(" << hits[i] << "/" << get_operations[i] << ")" << std::endl;
    }

    std::cout << std::endl; // 添加空行，使输出更清晰
}

void testHotDataAccess()
{
    std::cout << "\n=== 测试场景1：热点数据访问测试 ===" << std::endl;

    const int CAPACITY = 20;       // 缓存容量
    const int OPERATIONS = 500000; // 总操作次数
    const int HOT_KEYS = 20;       // 热点数据数量
    const int COLD_KEYS = 5000;    // 冷数据数量

    MyCache::LruCache<int, std::string> lru(CAPACITY);
    MyCache::LfuCache<int, std::string> lfu(CAPACITY);
    MyCache::ArcCache<int, std::string> arc(CAPACITY/2);
    // 为LRU-K设置合适的参数：
    // - 主缓存容量与其他算法相同
    // - 历史记录容量设为可能访问的所有键数量
    // - k=2表示数据被访问2次后才会进入缓存，适合区分热点和冷数据
    MyCache::LruKCache<int, std::string> lruk(CAPACITY, HOT_KEYS + COLD_KEYS, 2);
    MyCache::LfuCache<int, std::string> lfuAging(CAPACITY, 20000);

    std::random_device rd;
    std::mt19937 gen(rd());

    // 基类指针指向派生类对象，添加LFU-Aging
    std::array<MyCache::CachePolicy<int, std::string> *, 5> caches = {&lru, &lfu, &arc, &lruk, &lfuAging};
    std::vector<int> hits(5, 0);
    std::vector<int> get_operations(5, 0);
    std::vector<std::string> names = {"LRU", "LFU", "ARC", "LRU-K", "LFU-Aging"};

    // 为所有的缓存对象进行相同的操作序列测试
    for (int i = 0; i < caches.size(); ++i)
    {
        // 先预热缓存，插入一些数据
        for (int key = 0; key < HOT_KEYS; ++key)
        {
            std::string value = "value" + std::to_string(key);
            caches[i]->put(key, value);
        }

        // 交替进行put和get操作，模拟真实场景
        for (int op = 0; op < OPERATIONS; ++op)
        {
            // 大多数缓存系统中读操作比写操作频繁
            // 所以设置30%概率进行写操作
            bool isPut = (gen() % 100 < 30);
            int key;

            // 70%概率访问热点数据，30%概率访问冷数据
            if (gen() % 100 < 70)
            {
                key = gen() % HOT_KEYS; // 热点数据
            }
            else
            {
                key = HOT_KEYS + (gen() % COLD_KEYS); // 冷数据
            }

            if (isPut)
            {
                // 执行put操作
                std::string value = "value" + std::to_string(key) + "_v" + std::to_string(op % 100);
                caches[i]->put(key, value);
            }
            else
            {
                // 执行get操作并记录命中情况
                std::string result;
                get_operations[i]++;
                if (caches[i]->get(key, result))
                {
                    hits[i]++;
                }
            }
        }
    }

    // 打印测试结果
    printResults("热点数据访问测试", CAPACITY, get_operations, hits);
}
int main()
{
    testHotDataAccess();

    return 0;
}