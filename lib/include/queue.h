#pragma once

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
 
template <typename T>
class Queue
{
 private:
  std::queue<T> _queue;
  std::mutex _mutex;
  std::condition_variable _cond;
  
 public:
  T pop()
  {
    std::unique_lock<std::mutex> mlock(_mutex);
    while (_queue.empty()) {
      _cond.wait(mlock);
    }
    auto item = std::move(_queue.front());
    _queue.pop();
    return item;
  }
 
  void push(const T& item)
  {
    std::unique_lock<std::mutex> mlock(_mutex);
    _queue.push(item);
    mlock.unlock();
    _cond.notify_one();
  }
  
  void push(T&& item)
  {
    std::unique_lock<std::mutex> mlock(_mutex);
    _queue.push(std::move(item));
    mlock.unlock();
    _cond.notify_one();
  }
  
  bool empty() const {
    return _queue.empty();
  }
  
  size_t size() const {
    return _queue.size();
  }
};
