//
// Created by cha on 2022/3/11.
//

#ifndef MYSQL2REDIS__RESERVE_DATA_H_
#define MYSQL2REDIS__RESERVE_DATA_H_

#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "line_data.h"
#include "glog/logging.h"
#include "redis_connector.h"
#include "config.h"
#include "thread_pool.h"

class ImportBase {
 public:
  virtual void SendData() = 0;
  virtual int PutLineData(std::shared_ptr<LineData> line_data) = 0;
};

class RedisReserve : public ImportBase {
 public:

  RedisReserve();

  ~RedisReserve() = default;

  void SendData() override;

  int PutLineData(std::shared_ptr<LineData> line_data) override;

 private:
  std::unique_ptr<RedisConnector> redis_connectors_;
  std::map<std::string, RedisImportConfig> import_config_;
  std::queue<std::shared_ptr<LineData>> line_data_queue_;
  std::shared_ptr<LineData> cur_data_;
  std::mutex mutex_;
  std::condition_variable cond_;
};

#endif //MYSQL2REDIS__RESERVE_DATA_H_
