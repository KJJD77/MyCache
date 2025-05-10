cmake_minimum_required(VERSION 3.10)

# 项目名称
project(MyCache)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加源文件
set(SOURCES
    test.cpp
    ArcCache/ArcCache.hpp
    ArcCache/ArcLruPart.hpp
    ArcCache/ArcLfuPart.hpp
    ArcCache/ArcCacheNode.hpp
    LruCache.hpp
    LfuCache.hpp
    CachePolicy.h
)

# 包含头文件目录
include_directories(${CMAKE_SOURCE_DIR})

# 添加可执行文件
add_executable(MyCacheTest ${SOURCES})

# 添加线程库（如果需要）
find_package(Threads REQUIRED)
target_link_libraries(MyCacheTest PRIVATE Threads::Threads)