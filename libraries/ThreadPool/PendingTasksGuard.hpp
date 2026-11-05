/// \file PendingTasksGuard.hpp
/// \brief Declaration of \ref PendingTasksGuard, which waits for the pending tasks of a \ref ThreadPool before the objects they use are destroyed.

#pragma once

#include <atomic>
#include <future>
#include <stdexcept>
#include <vector>

/// \brief Keeps the objects used by thread-pool tasks alive until those tasks have finished.
///
/// Tasks submitted to a \ref ThreadPool (for example the global \ref threadPool) often hold pointers and references to local
/// objects of the caller (in the SFE programs: fuzzy extractors and samples). If the caller leaves the scope of those
/// objects (an early \c return or an exception after one task failed) while other tasks are still queued or running, those
/// tasks use destroyed objects. Declare the guard right after the futures
/// vector and after every object the tasks use, so it is destroyed first: its destructor cancels the tasks that have not
/// started yet and waits for every pending future. On the normal path all futures have already been consumed with \c get(),
/// so the destructor does nothing.
template <typename T>
class PendingTasksGuard
{
public:
    /// Guards the futures of \p futures, which must outlive the guard.
    /// @param futures Futures of the submitted tasks.
    explicit PendingTasksGuard(std::vector<std::future<T>> &futures) : m_futures(futures), m_cancelled(false) {}
    /// Not copyable.
    PendingTasksGuard(const PendingTasksGuard &) = delete;
    /// Not copy-assignable.
    PendingTasksGuard &operator=(const PendingTasksGuard &) = delete;
    /// Cancels the tasks that have not started yet (see \ref cancellable) and waits for every valid future.
    ~PendingTasksGuard()
    {
        m_cancelled = true;
        for (auto &future : m_futures) {
            if (future.valid()) {
                future.wait();
            }
        }
    }
    /// Wraps \p task so that it is skipped (its future then holds an exception) if it starts after the guard was triggered.
    /// @tparam F Callable type.
    /// @param task Callable to run.
    /// @return Callable that throws \c std::runtime_error instead of running \p task once the guard has been destroyed.
    template <typename F>
    auto cancellable(F task)
    {
        return [this, task]() mutable {
            if (m_cancelled) {
                throw std::runtime_error("Task cancelled because another task failed.");
            }
            return task();
        };
    }
private:
    /// Futures of the guarded tasks.
    std::vector<std::future<T>> &m_futures;
    /// Set by the destructor: the tasks that start afterwards are skipped.
    std::atomic<bool> m_cancelled;
};
