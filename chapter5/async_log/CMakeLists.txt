cmake_minimum_required(VERSION 3.5)
project(async_log)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_subdirectory(base)
set(SOURCE_FILE ProcessInfo_test.cpp)
add_executable(ProcessInfo_test ${SOURCE_FILE})
target_link_libraries(ProcessInfo_test base)
add_executable(Atomic_unittest Atomic_unittest.cpp)
target_link_libraries(Atomic_unittest base)
add_executable(Timestamp_unittest Timestamp_unittest.cpp)
target_link_libraries(Timestamp_unittest base)
add_executable(Thread_test Thread_test.cpp)
target_link_libraries(Thread_test base)
add_executable(Logging_test Logging_test.cpp)
target_link_libraries(Logging_test base)
add_executable(AsyncLogging_test AsyncLogging_test.cpp)
target_link_libraries(AsyncLogging_test base)

