ThreadPool library {#threadpool_page}
==================

[TOC]

ThreadPool is a small C++14 library that runs independent tasks on a fixed set of worker threads. The SFE programs use it to
enroll many subjects, and to run many template–probe comparisons, in parallel: each enrollment or reproduction is a
self-contained task that only reads shared, immutable data (the general configuration and the normalizers) and writes its own
fuzzy-extractor object.

\author Enrique Argones Rúa (COSIC, KU Leuven)
\date 2025

Design {#threadpool_design}
======

The library follows the classical *thread pool* pattern \cite williams2019concurrency, which works as follows:

- \ref SafeQueue is a FIFO queue of type-erased tasks (\c std::function<void()>) protected by a mutex. It is not copyable.
- \ref ThreadPool owns \f$n\f$ worker threads created by \ref ThreadPool::init. Each worker waits on a condition variable until
  a task is queued or the pool shuts down, dequeues one task and runs it.
- \ref ThreadPool::submit binds a callable and its arguments, wraps them into a \c std::packaged_task and returns the
  corresponding \c std::future. The future delivers the return value of the task, or rethrows the exception that the task
  threw, in the thread that calls \c get(). This is how errors raised inside the fuzzy extractors (for example a covariance
  matrix that cannot be inverted) reach the programs.
- Enqueuing a task and checking for pending work are both done while holding the mutex of the condition variable, and the
  workers wait with a predicate, so a wake-up can never be lost.
- \ref ThreadPool::shutdown stops the workers after their current task. Tasks still in the queue are discarded, and their
  futures report \c std::future_errc::broken_promise. Submitting to a pool that has been shut down throws
  \c std::runtime_error.

Two global pools are defined once in ThreadPool.cpp and shared by all the code linked with the library: \ref threadPool, with
\ref MAX_THREADS workers, and \ref threadPool_sequential, with a single worker (tasks run one after the other, in submission
order).

\code{.cpp}
#include <ThreadPool/ThreadPool.hpp>

std::future<void> f = threadPool.submit(&HybridStrengthenedTurboCodeFuzzyExtractor::generate, &hstcfe, std::ref(samples));
f.get();   // waits for completion and rethrows any exception thrown by the task
\endcode

\ref PendingTasksGuard (header \c PendingTasksGuard.hpp) protects the caller when one of several tasks fails: declared after
the vector of futures and after the objects the tasks use, it is destroyed first, cancels the tasks that have not started yet
and waits for the running ones, so that no task uses an object that has already been destroyed. Both SFE programs use it.

\warning A task must not wait for the result of another task submitted to the same pool: if every worker is blocked that way,
the tasks they wait for never run.

Building {#threadpool_build}
========

The library depends only on the C++ standard library and POSIX threads (CMake's \c Threads package). It is built by the
top-level CMake project of the repository, or on its own:

    cmake -S libraries/ThreadPool -B build/ThreadPool -DCMAKE_BUILD_TYPE=Release \
          [-DCMAKE_INSTALL_PREFIX=PREFIX]
    cmake --build build/ThreadPool -j
    cmake --install build/ThreadPool

It is installed as \c libThreadPool (\c libThreadPool_Debug in Debug builds), with its headers in
\c PREFIX/include/ThreadPool.
