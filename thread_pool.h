//
// Created by cha on 2022/3/11.
//

#ifndef MYSQL2REDIS__THREAD_POOL_H_
#define MYSQL2REDIS__THREAD_POOL_H_

#include <thread>
#include <functional>
#include <mutex>
#include <queue>
#include <condition_variable>

class BlockingQueue {
 public:
  using Task = std::function<void()>;

  BlockingQueue() = default;

  void put(const Task &task);

  void put(Task &&task);

  Task take();

  size_t size() const;

 private:
  mutable std::mutex mutex_;
  std::queue<Task> queue_;
  std::condition_variable cond_;
};

class ThreadPool {
 private:
  ThreadPool() = default;
  ~ThreadPool() = default;
 public:
  ThreadPool(const ThreadPool &other) = delete;
  ThreadPool &operator=(const ThreadPool &other) = delete;

  static ThreadPool &GetThreadPoolInstance() {
    static ThreadPool instance;
    return instance;
  }

  void Init(int num);

  void put(const BlockingQueue::Task &task);

  void put(BlockingQueue::Task &&task);

  void runInThread();

 private:

  std::vector<std::unique_ptr<std::thread>> threads_;
  BlockingQueue blocking_queue_;
};
#endif //MYSQL2REDIS__THREAD_POOL_H_
