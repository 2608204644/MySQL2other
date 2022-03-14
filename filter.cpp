//
// Created by cha on 2022/3/10.
//

#include "filter.h"

int Filter::FilterData(std::shared_ptr<LineData> &line_data) {
  std::string key = line_data->GetDatabaseTableName();
  if (filter_configs_.find(key) == filter_configs_.end()) {
    line_data->SetDiscard(true);
    return 0;
  }
  auto &filter = filter_configs_[key];
  line_data->ReserveSomeColumn(filter.reserve_columns);
  return 0;
}
