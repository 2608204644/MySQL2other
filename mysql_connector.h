//
// Created by cha on 2022/3/10.
//

#ifndef MYSQL2REDIS__MYSQL_CONNECTOR_H_
#define MYSQL2REDIS__MYSQL_CONNECTOR_H_

#include <fcntl.h>
#include <mysql/mysql.h>
#include <glog/logging.h>
#include <errno.h>
#include "config.h"
#include "util.h"

class MysqlConnector {
 public:
  MysqlConnector();
  ~MysqlConnector();

  int init(const MysqlConnectConfig &config);

  /*
   *@todo 连接数据库
   */
  int Connect();

  /*
   *@todo 断开数据库连接
   */
  int DisConnect();

  /*
   *@todo 保持数据库连接
   */
  int KeepAlive();

  /*
   *@todo 判断与mysql交互的协议是否启用了checksum，mysql5.6新加入checksum机制
   */
  bool CheckIsChecksumEnable();

  /*
   *@todo 获取到数据库中binlog的最新位置
   *@param szBinlogFileName binlog文件名
   *@param puBinlogPos binlog文件的偏移量
   */
  int GetNowBinlogPos(string &binlog_file_name, uint32_t &binlog_pos);

  /*
   *@todo 获取mysql的文件描述符
   */
  int GetFd() const;

  /*
   *@todo 向数据库服务器发送数据报,请求服务器将binlog发过来
   */
  int ReqBinlog(string &binlog_file_name, uint32_t binlog_pos, bool checksum_enable);

 private:
  MYSQL mysql;
  time_t pre_keep_alive_time;
  bool connected;
  MysqlConnectConfig connect_config;

};

#endif //MYSQL2REDIS__MYSQL_CONNECTOR_H_
