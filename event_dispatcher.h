#ifndef MYSQL2REDIS__EVENT_DISPATCHER_H_
#define MYSQL2REDIS__EVENT_DISPATCHER_H_

#include <memory>
#include <vector>
#include "reserve_data.h"
#include "thread_pool.h"

class EventDispatcher {
 private:
  EventDispatcher() = default;
  ~EventDispatcher() = default;
 public:
  EventDispatcher(const EventDispatcher &other) = delete;
  EventDispatcher &operator=(const EventDispatcher &other) = delete;

  static EventDispatcher &GetEventDispatcherInstance() {
    static EventDispatcher instance;
    return instance;
  }
  void PutLineData(std::shared_ptr<LineData> data);
  void PutImportBase(ImportBase *base) { import_bases.push_back(base); }
 private:
  std::vector<ImportBase *> import_bases;
};

#endif //MYSQL2REDIS__EVENT_DISPATCHER_H_
