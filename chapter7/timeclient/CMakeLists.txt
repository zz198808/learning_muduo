cmake_minimum_required(VERSION 3.5)
project(timeclient)
find_package(Threads)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(/home/zhouyang/Documents/muduo/include)
link_directories(/home/zhouyang/Documents/muduo/lib)
set(SOURCE_FILES main.cpp)
add_executable(timeclient ${SOURCE_FILES})
target_link_libraries( timeclient muduo_net muduo_base ${CMAKE_THREAD_LIBS_INIT} )