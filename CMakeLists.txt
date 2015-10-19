cmake_minimum_required(VERSION 2.6)
project(httpd)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

set(SOURCE_FILES main.c threadpool/thread_pool.c threadpool/thread_pool.h threadpool/binary_semaphore.c threadpool/binary_semaphore.h threadpool/job.h threadpool/http.c threadpool/http.h)
add_executable(httpd ${SOURCE_FILES})

#find_library(LIBEVENT_LIB event HINTS /usr/lib/x86_64-linux-gnu)
find_library(LIBEVENT_LIB event)
find_library(PTHREAD_LIB pthread)
find_library(LIBEVENT_PTHREADS_LIB event_pthreads)
target_link_libraries(httpd ${LIBEVENT_LIB})
target_link_libraries(httpd ${PTHREAD_LIB})
target_link_libraries(httpd ${LIBEVENT_PTHREADS_LIB})