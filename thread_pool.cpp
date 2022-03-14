#include "thread_pool.h"

void BlockingQueue::put(const Task &task) {
  std::lock_guard<std::mutex> lock(mutex_);
  queue_.emplace(task);
  cond_.notify_one();
}

void BlockingQueue::put(Task &&task) {
  std::lock_guard<std::mutex> lock(mutex_);
  queue_.emplace(std::move(task));
  cond_.notify_one();
}

BlockingQueue::Task BlockingQueue::take() {
  std::unique_lock<std::mutex> lock(mutex_);
  cond_.wait(lock, [this]() { return queue_.size() > 0; });
  Task task(std::move(queue_.front()));
  queue_.pop();
  return std::move(task);
}
size_t BlockingQueue::size() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return queue_.size();
}

void ThreadPool::Init(int num) {
  threads_.reserve(num);
  for (int i = 0; i < num; ++i) {
    std::unique_ptr<std::thread> ptr(new std::thread([this] { runInThread(); }));
    threads_.emplace_back(std::move(ptr));
  }
}

void ThreadPool::runInThread() {
  while (true) {
    BlockingQueue::Task task(blocking_queue_.take());
    task();
  }
}

void ThreadPool::put(const BlockingQueue::Task &task) {
  blocking_queue_.put(task);
}

void ThreadPool::put(BlockingQueue::Task &&task) {
  blocking_queue_.put(task);
}
