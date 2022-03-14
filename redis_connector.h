#ifndef MYSQL2REDIS__REDIS_CONNECTOR_H_
#define MYSQL2REDIS__REDIS_CONNECTOR_H_

#include <hiredis/hiredis.h>
#include <string>
#include "config.h"
#include "glog/logging.h"

class RedisConnector {
 public:

  RedisConnector(RedisConnectConfig connect_config);
  ~RedisConnector();

  //建立redis连接 成功返回0  失败返回-1
  int Connect();

  //执行redis语句 根据返回值reply 判断是否成功 成功返回0 失败返回-1(reply为空 或redis连接失败) -2(只读) -3(脚本加载错误) -4(其他)
  int ExecuteRedisCmd(const char *pszFormat, ...);

  int Reconnect();

 private:

  int Auth();

  inline int GetReplyInt(int &reply) const { return (reply_ ? (reply = reply_->integer, 0) : -1); }

  inline char *GetReplyStr() const { return (reply_ ? reply_->str : NULL); }

  inline redisReply *GetReply() const { return reply_; };

 private:
  redisReply *reply_;
  redisContext *redis_;
  RedisConnectConfig config_;
};

#endif //MYSQL2REDIS__REDIS_CONNECTOR_H_
