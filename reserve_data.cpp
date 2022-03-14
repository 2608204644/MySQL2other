#include "reserve_data.h"

RedisReserve::RedisReserve() {
  auto config = SConfig::GetConfigInstance().GetRedisConfig();
  redis_connectors_ = std::make_unique<RedisConnector>(config.connect_config);
  redis_connectors_->Connect();
  import_config_ = config.import_configs;
}
void RedisReserve::SendData() {
  std::unique_lock<std::mutex> lock(mutex_);
  cond_.wait(lock, [this]() { return line_data_queue_.size() > 0; });
  cur_data_ = line_data_queue_.front();
  line_data_queue_.pop();
  auto config = import_config_[cur_data_->GetDatabaseTableName()];
  auto key_column_index = cur_data_->GetColumnIndexByName(config.key);
  if (cur_data_->GetLineAction() == DELETE) {
    auto key = cur_data_->GetColumnByIndex(key_column_index, true);
    key = config.key_prefix + key;
    for (const auto &item: import_config_[cur_data_->GetDatabaseTableName()].values) {
      LOG(INFO) << __FUNCTION__ << " ExecuteRedisCmd hdel " << key << " " << item << std::endl;
      redis_connectors_->ExecuteRedisCmd("hdel %s %s", key.c_str(), item.c_str());
    }
  } else {
    auto key = cur_data_->GetColumnByIndex(key_column_index, false);
    key = config.key_prefix + key;
    for (const auto &item: import_config_[cur_data_->GetDatabaseTableName()].values) {
      auto index = cur_data_->GetColumnIndexByName(item);
      auto value = cur_data_->GetColumnByIndex(index, false);
      LOG(INFO) << __FUNCTION__ << " ExecuteRedisCmd hset " << key << " " << item << " " << value << std::endl;
      redis_connectors_->ExecuteRedisCmd("hset %s %s %s", key.c_str(), item.c_str(), value.c_str());
    }
  }
  return;
}

int RedisReserve::PutLineData(std::shared_ptr<LineData> line_data) {
  std::lock_guard<std::mutex> lock(mutex_);
  line_data_queue_.push(line_data);
  cond_.notify_one();
}
