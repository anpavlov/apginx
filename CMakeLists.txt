cmake_minimum_required(VERSION 2.6)
project(apginx)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.c threadpool/thread_pool.c threadpool/thread_pool.h threadpool/binary_semaphore.c threadpool/binary_semaphore.h threadpool/job_queue.c threadpool/job_queue.h threadpool/job.h)
add_executable(apginx ${SOURCE_FILES})

#find_library(LIBEVENT_LIB event HINTS /usr/lib/x86_64-linux-gnu)
find_library(LIBEVENT_LIB event)
target_link_libraries(apginx ${LIBEVENT_LIB})