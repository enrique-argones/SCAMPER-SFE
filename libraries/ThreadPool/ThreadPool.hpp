/// \file ThreadPool.hpp
/// \brief Fixed-size thread pool used to enroll and verify many templates in parallel, plus two global pool instances.

#pragma once

#include "ThreadPool_global.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
#include <utility>

// #include <iostream>

#include "SafeQueue.hpp"

/// Number of worker threads of the global \ref threadPool.
#define MAX_THREADS 24

/// Fixed-size pool of worker threads executing submitted callables in FIFO order.
///
/// \code
/// std::future<void> f = threadPool.submit(&HybridStrengthenedTurboCodeFuzzyExtractor::generate, &hstcfe, std::ref(samples));
/// f.get();   // waits for completion and rethrows any exception thrown by the task
/// \endcode
///
/// \par Synchronisation
/// Enqueuing a task (\ref submit) and checking for pending work in the worker loop are both done while holding
/// \ref m_conditional_mutex, and workers wait on \ref m_conditional_lock with a predicate. A notification can therefore
/// never be lost between a worker finding the queue empty and going to sleep.
///
/// \warning A task must not block waiting for the result of another task submitted to the \b same pool: if every
/// worker is blocked that way, the queued tasks they wait for never run (deadlock). Use a different pool for the inner
/// tasks.
class THREADPOOL_EXPORT ThreadPool {
private:
    /// Function object run by each worker thread: it waits for tasks and executes them until the pool shuts down.
    class ThreadWorker {
    private:
        /// Worker index within the pool.
        int m_id;
        /// Pool the worker belongs to.
        ThreadPool * m_pool;
    public:
        /// Constructor.
        /// @param pool Pool the worker belongs to.
        /// @param id Worker index.
        ThreadWorker(ThreadPool * pool, const int id)
            : m_id(id), m_pool(pool) {
        }

        /// Worker loop: waits until a task is queued or the pool shuts down, dequeues a task and runs it.
        /// Returns when \ref m_shutdown is set (tasks still queued at that point are not executed).
        void operator()();
    };

    /// Set to true by \ref shutdown to stop the workers. Written while holding \ref m_conditional_mutex.
    std::atomic<bool> m_shutdown;
    /// Queue of pending tasks.
    SafeQueue<std::function<void()>> m_queue;
    /// Worker threads.
    std::vector<std::thread> m_threads;
    /// Mutex associated with \ref m_conditional_lock. Held while enqueuing tasks and while workers check for work.
    std::mutex m_conditional_mutex;
    /// Condition variable used to wake up idle workers.
    std::condition_variable m_conditional_lock;
public:
    /// Creates the pool and starts its workers (\ref init).
    /// @param n_threads Number of worker threads.
    ThreadPool(const int n_threads);

    /// Not copyable.
    ThreadPool(const ThreadPool &) = delete;
    /// Not movable.
    ThreadPool(ThreadPool &&) = delete;

    /// Not copy-assignable.
    ThreadPool & operator=(const ThreadPool &) = delete;
    /// Not move-assignable.
    ThreadPool & operator=(ThreadPool &&) = delete;

    /// Destructor. Calls \ref shutdown.
    ~ThreadPool();

    /// Inits thread pool
    void init();

    /// Waits until threads finish their current task and shutdowns the pool.
    /// Tasks still queued are discarded: their futures report \c std::future_errc::broken_promise.
    /// Calling it more than once is harmless.
    void shutdown();

    /// Submit a function to be executed asynchronously by the pool
    /// @tparam F Callable type.
    /// @tparam Args Argument types.
    /// @param f Callable (function, member function pointer, lambda, ...).
    /// @param args Arguments bound to \p f (use \c std::ref to pass by reference).
    /// @return A \c std::future holding the result of the call, or the exception it threw.
    /// @throws std::runtime_error if the pool has already been shut down (the task would never run).
    template<typename F, typename...Args> auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>  {
        // Create a function with bounded parameters ready to execute
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        // Encapsulate it into a shared ptr in order to be able to copy construct / assign
        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

        // Wrap packaged task into void function
        std::function<void()> wrapper_func = [task_ptr]() {
            (*task_ptr)();
        };

        {
            // Enqueue under the condition-variable mutex, so that no worker can be between its
            // "queue empty?" check and its wait while the task is added (no lost wake-up).
            std::lock_guard<std::mutex> lock(m_conditional_mutex);
            if (m_shutdown) {
                throw std::runtime_error("ThreadPool::submit: the pool has been shut down.");
            }
            m_queue.enqueue(wrapper_func);
        }

        // Wake up one thread if its waiting
        m_conditional_lock.notify_one();

        // Return future from promise
        return task_ptr->get_future();
    }

};

/// Global pool with \ref MAX_THREADS workers, used by the command-line programs.
/// Defined once, in ThreadPool.cpp, so all the code linked with the library shares the same pool.
extern THREADPOOL_EXPORT ThreadPool threadPool;
/// Global single-worker pool (tasks run sequentially, in submission order). Defined once, in ThreadPool.cpp.
extern THREADPOOL_EXPORT ThreadPool threadPool_sequential;
