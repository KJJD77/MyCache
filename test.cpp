#include "LruCache.hpp"
#include<string>
int main()
{
    MyCache::LruCache<int,std::string> lru(10);
    MyCache::LruKCache<int,std::string> lruk(10,10,2);
    std::string val;
/*     for(int i=0;i<10;i++)
        lru.put(i,std::string("1")),std::cout<<lru.get(i,val)<<'\n',std::cout<<val<<'\n'; */
    for(int i=0;i<10;i++)
        lruk.put(i,std::string("a")),std::cout<<lruk.get(i)<<'\n'; 
    return 0;
}


