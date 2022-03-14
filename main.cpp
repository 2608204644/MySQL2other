#include <iostream>
#include <yaml-cpp/yaml.h>
#include <glog/logging.h>
#include "mysql_connector.h"
#include "event_dispatcher.h"
#include "reserve_data.h"
#include "thread_pool.h"
#include "config.h";
#include "factory.h"
#include "packet.h"

int main() {

  //set glog
  google::SetLogDestination(0, "../log/info.log");
  google::InitGoogleLogging("log_test");

  //set config
  SConfig::GetConfigInstance();

  //set Mysql connect
  MysqlConnector mysql_connector;
  mysql_connector.init(SConfig::GetConfigInstance().GetMysqlConfig().connect_config);
  while (mysql_connector.Connect() == -1) { sleep(2); };
  string binlog_filename;
  uint32_t binlog_pos;
  bool binlog_checksum_enable;
  mysql_connector.GetNowBinlogPos(binlog_filename, binlog_pos);
  binlog_checksum_enable = mysql_connector.CheckIsChecksumEnable();


  //set threadPool class
  ThreadPool::GetThreadPoolInstance().Init(5);

  //register product to factory class
  ProductRegistrar<ImportBase, RedisReserve> redis_registrar("redis");
  auto redis_reserve = ProductFactory<ImportBase>::Instance().GetProduct("redis");

  //set event dispatcher class
  EventDispatcher::GetEventDispatcherInstance().PutImportBase(redis_reserve);

  //set Filter class
  Filter::GetFilterInstance().Init(SConfig::GetConfigInstance().GetMysqlConfig().filter_configs);

  //set packet class
  Packet packet;
  packet.Init(mysql_connector.CheckIsChecksumEnable());

  int retry_cnt = 0;
  while (true) {
    mysql_connector.ReqBinlog(binlog_filename, binlog_pos, binlog_checksum_enable);

    //loop for get mysqlfd;
    {
      int fd = mysql_connector.GetFd();
      int ret = 0;
      while (true) {
        packet.Read(fd);
        ret = packet.Parse();
        if (ret == -1)break;
      }
    }

    mysql_connector.DisConnect();
    int res = mysql_connector.Connect();
    if (res != 0)return -1;

    if (retry_cnt++ > 3) {
      mysql_connector.GetNowBinlogPos(binlog_filename, binlog_pos);
      LOG(INFO) << "switch binlog File :" << binlog_filename << " Pos:" << binlog_pos << std::endl;
      retry_cnt = 0;
    }
  }

  return 0;
}
