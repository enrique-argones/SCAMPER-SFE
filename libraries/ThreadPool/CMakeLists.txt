cmake_minimum_required(VERSION 3.23)   # FILE_SET HEADERS needs CMake 3.23

project(ThreadPool VERSION 1.0.0 LANGUAGES CXX)

# ThreadPool: fixed-size thread pool with a thread-safe task queue.
include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/SFEModule.cmake)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(ThreadPool SHARED
    ThreadPool_global.h
    ThreadPool.cpp
    ThreadPool.hpp
    SafeQueue.hpp
    PendingTasksGuard.hpp
)
sfe_module_target(ThreadPool)

# Only the symbols marked with THREADPOOL_EXPORT are exported (-fvisibility=hidden).
target_compile_options(ThreadPool PRIVATE -pipe -fvisibility=hidden -Wall -Wextra -Wno-deprecated)
target_compile_definitions(ThreadPool PRIVATE THREADPOOL_LIBRARY _REENTRANT)
target_link_libraries(ThreadPool PUBLIC Threads::Threads)

target_sources(ThreadPool
    PUBLIC
        FILE_SET HEADERS
        TYPE HEADERS
        BASE_DIRS "./"
        FILES ThreadPool_global.h ThreadPool.hpp SafeQueue.hpp PendingTasksGuard.hpp
)

install(TARGETS ThreadPool
        FILE_SET HEADERS
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ThreadPool
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
