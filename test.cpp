#include "LruCache.hpp"
#include<string>
int main()
{
    MyCache::LruCache<int,int> lru(10);
    for(int i=0;i<10;i++)
        lru.put(i,i),std::cout<<lru.get(i+1)<<'\n';
    return 0;
}


