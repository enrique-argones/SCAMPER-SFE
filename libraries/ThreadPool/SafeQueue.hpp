/// \file SafeQueue.hpp
/// \brief Mutex-protected FIFO queue used as the task queue of \ref ThreadPool.

#pragma once

#include <mutex>
#include <queue>

/// Thread safe implementation of a Queue using an std::queue.
/// Every operation locks \ref m_mutex for its whole duration.
/// @tparam T Type of the queued elements.
template <typename T>
class SafeQueue {
private:
  /// Underlying queue.
  std::queue<T> m_queue;
  /// Mutex protecting \ref m_queue.
  std::mutex m_mutex;
public:
  /// Constructs an empty queue.
  SafeQueue() {

  }

  /// Not copyable (\c std::mutex cannot be copied, and \ref ThreadPool, the only user, is not copyable either).
  SafeQueue(const SafeQueue &) = delete;
  /// Not copy-assignable.
  SafeQueue & operator=(const SafeQueue &) = delete;

  /// Destructor.
  ~SafeQueue() {

  }


  /// @return true if the queue holds no elements.
  bool empty() {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_queue.empty();
  }
  
  /// @return Number of queued elements.
  int size() {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_queue.size();
  }

  /// Appends an element at the back of the queue.
  /// @param[in] t Element to append (copied).
  void enqueue(T& t) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue.push(t);
  }
  
  /// Removes the element at the front of the queue.
  /// @param[out] t Receives the removed element (moved) if the queue was not empty.
  /// @return true if an element was removed, false if the queue was empty.
  bool dequeue(T& t) {
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_queue.empty()) {
      return false;
    }
    t = std::move(m_queue.front());
    
    m_queue.pop();
    return true;
  }
};