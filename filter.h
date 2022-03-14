//
// Created by cha on 2022/3/10.
//

#ifndef MYSQL2REDIS__FILTER_H_
#define MYSQL2REDIS__FILTER_H_

#include <string>
#include <utility>
#include <memory>
#include "config.h"
#include "line_data.h"

class Filter {
 private:
  Filter() = default;
  ~Filter() = default;
 public:
  Filter(const Filter &other) = delete;
  Filter &operator=(const Filter &other) = delete;
  void Init(std::map<std::string, MysqlFilterConfig> configs) { filter_configs_ = std::move(configs); }

  int FilterData(std::shared_ptr<LineData> &line_data);

  static Filter &GetFilterInstance() {
    static Filter instance;
    return instance;
  }

 private:
  std::map<std::string, MysqlFilterConfig> filter_configs_;
};

#endif //MYSQL2REDIS__FILTER_H_
