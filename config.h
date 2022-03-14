#ifndef MYSQL2REDIS_CONFIG_H
#define MYSQL2REDIS_CONFIG_H

#include <string>
#include <vector>
#include "yaml-cpp/yaml.h"
#include <glog/logging.h>

using std::string;

struct MysqlConnectConfig {
  std::string ip;
  uint32_t port{};
  std::string username;
  std::string password;
  bool need_decode{};
  uint32_t server_id{};
};

struct MysqlFilterConfig {
  std::string database_name;
  std::string table_name;
  std::map<uint32_t, std::string> reserve_columns;
};

struct Mysql {
  MysqlConnectConfig connect_config;
  std::map<std::string, MysqlFilterConfig> filter_configs;
};

struct RedisConnectConfig {
  std::string ip;
  uint32_t port{};
  bool need_password{};
  std::string password;
};

struct RedisImportConfig {
  std::string db_table;
  std::string key_prefix;
  std::string key;
  std::vector<std::string> values;
};

struct Redis {
  RedisConnectConfig connect_config;
  std::map<std::string, RedisImportConfig> import_configs;
};

class SConfig {
 private:
  SConfig() {
    LoadConfig();
  };
  int LoadConfig();
  ~SConfig() = default;
 public:
  SConfig(const SConfig &) = delete;

  SConfig &operator=(const SConfig &) = delete;

  static SConfig &GetConfigInstance() {
    //返回local static的引用
    static SConfig instance;
    return instance;
  }

 public:
  const Mysql &GetMysqlConfig() { return mysql_config_; }
  const Redis &GetRedisConfig() { return redis_config_; }
  friend std::ostream &operator<<(std::ostream &os, const SConfig &config);
 private:
  Mysql mysql_config_;
  Redis redis_config_;

};

std::ostream &operator<<(std::ostream &os, const MysqlConnectConfig &config);
std::ostream &operator<<(std::ostream &os, const MysqlFilterConfig &config);
std::ostream &operator<<(std::ostream &os, const Mysql &config);

std::ostream &operator<<(std::ostream &os, const RedisConnectConfig &config);
std::ostream &operator<<(std::ostream &os, const RedisImportConfig &config);
std::ostream &operator<<(std::ostream &os, const Redis &config);

#endif //MYSQL2REDIS_CONFIG_H
