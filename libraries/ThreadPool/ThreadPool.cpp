/// \file ThreadPool.cpp
/// \brief Implementation of the \ref ThreadPool worker loop, start-up and shutdown, and definition of the global pools.

#include "ThreadPool.hpp"

// The only definitions of the global pools: every program and library linked with libThreadPool shares them.
ThreadPool threadPool(MAX_THREADS);
ThreadPool threadPool_sequential(1);

// ThreadPool::ThreadPool() {}

void ThreadPool::ThreadWorker::operator()() {
    std::function<void()> func;
    while (true) {
        {
            std::unique_lock<std::mutex> lock(m_pool->m_conditional_mutex);
            // The predicate is evaluated under the mutex, so a task enqueued (also under the mutex) between the
            // check and the wait cannot be missed, and spurious wake-ups are ignored.
            m_pool->m_conditional_lock.wait(lock, [this] {
                return m_pool->m_shutdown || !m_pool->m_queue.empty();
            });
            if (m_pool->m_shutdown) {
                return;
            }
            m_pool->m_queue.dequeue(func);
        }
        func();
    }
}

ThreadPool::ThreadPool(const int n_threads)
    : m_shutdown(false), m_threads(std::vector<std::thread>(n_threads)) {
    init();
}

ThreadPool::~ThreadPool() {
    shutdown();
}

// Inits thread pool
void ThreadPool::init() {
    for (std::vector<std::thread>::size_type i = 0; i < m_threads.size(); ++i) {
        m_threads[i] = std::thread(ThreadWorker(this, i));
    }
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_conditional_mutex);
        m_shutdown = true;
    }
    m_conditional_lock.notify_all();

    for (std::vector<std::thread>::size_type i = 0; i < m_threads.size(); ++i) {
        if(m_threads[i].joinable()) {
            m_threads[i].join();
        }
    }
}
