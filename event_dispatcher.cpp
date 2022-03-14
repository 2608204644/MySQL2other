//
// Created by cha on 2022/3/12.
//

#include "event_dispatcher.h"

void EventDispatcher::PutLineData(std::shared_ptr<LineData> data) {
  for (const auto &item: import_bases) {
    item->PutLineData(data);
    ThreadPool::GetThreadPoolInstance().put([item]{item->SendData();});
  }
}
