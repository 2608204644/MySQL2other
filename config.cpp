#include "config.h"

static void LoadMysqlConnectConfig(const YAML::Node &node, MysqlConnectConfig &connect_config) {
  connect_config.ip = node["mysql_ip"].as<std::string>();
  connect_config.port = node["mysql_port"].as<uint32_t>();
  connect_config.username = node["username"].as<std::string>();
  connect_config.password = node["password"].as<std::string>();
  connect_config.need_decode = node["need_decode"].as<bool>();
  connect_config.server_id = node["server_id"].as<uint32_t>();
}

static void LoadMysqlFilterConfig(const YAML::Node &node, MysqlFilterConfig &filter_config) {
  filter_config.database_name = node["database"].as<std::string>();
  filter_config.table_name = node["table"].as<std::string>();
  for (const auto &item: node["reserve_column_name"]) {
    filter_config.reserve_columns[item["index"].as<uint32_t>()] = item["name"].as<std::string>();
  }
}

static void LoadMysqlConfig(const YAML::Node &node, Mysql &mysql) {
  LoadMysqlConnectConfig(node["connect"], mysql.connect_config);
  for (const auto &item: node["filter"]) {
    MysqlFilterConfig filter_config;
    std::string key = item["database"].as<std::string>() + "_" + item["table"].as<std::string>();
    LoadMysqlFilterConfig(item, filter_config);
    mysql.filter_configs[key] = std::move(filter_config);
  }
}

static void LoadRedisConnectConfig(const YAML::Node &node, RedisConnectConfig &connect_config) {
  connect_config.ip = node["redis_ip"].as<std::string>();
  connect_config.port = node["redis_port"].as<uint32_t>();
  connect_config.need_password = node["need_passwd"].as<bool>();
  connect_config.password = node["password"].as<std::string>();
}

static void LoadRedisImportConfig(const YAML::Node &node, RedisImportConfig &import_config) {
  import_config.db_table = node["db_table"].as<std::string>();
  import_config.key_prefix = node["key_prefix"].as<std::string>();
  import_config.key = node["key_name"].as<std::string>();
  for (auto i = node["value"].begin(); i != node["value"].end(); ++i) {
    import_config.values.push_back(i->as<std::string>());
  }
}

static void LoadRedisConfig(const YAML::Node &node, Redis &redis) {
  LoadRedisConnectConfig(node["connect"], redis.connect_config);
  for (const auto &item: node["import"]) {
    RedisImportConfig import_config;
    LoadRedisImportConfig(item, import_config);
    redis.import_configs[import_config.db_table] = std::move(import_config);
  }
}

int SConfig::LoadConfig() {
  YAML::Node config = YAML::LoadFile("../config.yaml");
  LoadMysqlConfig(config["mysql"], mysql_config_);
  LoadRedisConfig(config["redis"], redis_config_);
  LOG(INFO) << *this;
  return 0;
}

std::ostream &operator<<(std::ostream &os, const MysqlConnectConfig &config) {
  os << "  " << " mysql_connect_config: " << std::endl
     << "    " << " ip: " << config.ip << " port: " << config.port
     << " username: " << config.username << " password: " << config.password
     << " serve_id: " << config.server_id
     << std::endl;
  return os;
}
std::ostream &operator<<(std::ostream &os, const MysqlFilterConfig &config) {
  os << "  " << " mysql_filter_config " << std::endl
     << "    " << " database_name: " << config.database_name << " table_name: " << config.table_name
     << " reserve_column: " << std::endl;
  for (const auto &item: config.reserve_columns)os << "      " << item.first << " " << std::endl;
  return os;
}

std::ostream &operator<<(std::ostream &os, const Mysql &config) {
  os << " mysql_config_: " << std::endl
     << config.connect_config;
  for (const auto &item: config.filter_configs)os << item.second;
  return os;
}

std::ostream &operator<<(std::ostream &os, const RedisConnectConfig &config) {
  os << "  " << " redis_connect_config: " << std::endl
     << "    " << " ip: " << config.ip << " port: " << config.port
     << " need_passwd: " << config.need_password << " password: " << config.password << std::endl;
  return os;
}

std::ostream &operator<<(std::ostream &os, const RedisImportConfig &config) {
  os << "  " << " redis_import_config" << std::endl
     << "    " << " db_table: " << config.db_table
     << " key_prefix: " << config.key_prefix << " key: " << config.key;
  for (const auto &item: config.values)os << item << " ";
  return os;
}

std::ostream &operator<<(std::ostream &os, const Redis &config) {
  os << " redis_config_: " << std::endl
     << config.connect_config;
  for (const auto &item: config.import_configs)os << item.second << " ";
  return os;
}

std::ostream &operator<<(std::ostream &os, const SConfig &config) {
  os << std::endl << config.mysql_config_ << config.redis_config_;
  return os;
}




