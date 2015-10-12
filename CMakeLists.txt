cmake_minimum_required(VERSION 2.6)
project(apginx)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ")

set(SOURCE_FILES main.c threadpool/thread_pool.c threadpool/thread_pool.h threadpool/binary_semaphore.c threadpool/binary_semaphore.h threadpool/job.h)
add_executable(apginx ${SOURCE_FILES})

#find_library(LIBEVENT_LIB event HINTS /usr/lib/x86_64-linux-gnu)
find_library(LIBEVENT_LIB event)
find_library(PTHREAD_LIB pthread)
find_library(LIBEVENT_PTHREADS_LIB event_pthreads)
target_link_libraries(apginx ${LIBEVENT_LIB})
target_link_libraries(apginx ${PTHREAD_LIB})
target_link_libraries(apginx ${LIBEVENT_PTHREADS_LIB})