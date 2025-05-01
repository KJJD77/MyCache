#include "LruCache.hpp"
#include "LfuCache.hpp"
#include <string>
#include <iostream>
int main()
{
    MyCache::LruCache<int, std::string> lru(10);
    MyCache::LruKCache<int, std::string> lruk(10, 10, 2);
    MyCache::LfuCache<int, std::string> lfu(100, 5);
    std::string val;
    /*     for(int i=0;i<10;i++)
            lru.put(i,std::string("1")),std::cout<<lru.get(i,val)<<'\n',std::cout<<val<<'\n'; */
    for (int i = 0; i < 10; i++)
        lfu.put(i, std::string(1,char(i+'a'))), std::cout << lfu.get(std::max(0,i-1)) << '\n';
    return 0;
}
