cmake_minimum_required(VERSION 3.16)
project(MemoryAllocationAlgo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(dp "./Dynamic-partition-alloc/dynamic_partition.cpp" "./Dynamic-partition-alloc/test.hpp")
add_executable(pr "./Page-replacement/page_replacement.cpp")
