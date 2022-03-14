#include "redis_connector.h"

#include <utility>
RedisConnector::RedisConnector(RedisConnectConfig connect_config)
    : reply_(nullptr), redis_(nullptr), config_(std::move(connect_config)) {}

RedisConnector::~RedisConnector() {
  if (redis_)delete redis_;
  if (reply_) delete reply_;
}

int RedisConnector::Auth() {
  if (config_.need_password) {
    reply_ = (redisReply *) redisCommand(redis_, "AUTH %s", config_.password.c_str());
    if (reply_->type == REDIS_REPLY_ERROR) {
      LOG(ERROR) << "Redis Connect Fail : identity authentication password: " << config_.password << std::endl;
      return -1;
    }
    LOG(ERROR) << "Redis  identity authentication succeed <ip " << config_.ip << "port: " << config_.port << ">"
               << std::endl;
    freeReplyObject(reply_);
    reply_ = nullptr;
  }
  return 0;
}
int RedisConnector::Connect() {
  int ret = -1;
  if (config_.ip.empty() || 0 == config_.port) {
    LOG(ERROR) << "Redis Connect Fail invalid parameter ip " << config_.ip << "port: " << config_.port << std::endl;
    return ret;
  }

  redis_ = redisConnect(config_.ip.c_str(), (int) config_.port);
  if (nullptr == redis_ || redis_->err) {
    if (redis_) {
      redisFree(redis_);
      redis_ = nullptr;
    }
    LOG(ERROR) << "Connect to redisServer fail" << config_.ip << "port: " << config_.port << std::endl;
    return ret;
  }

  ret = Auth();
  if (ret == 0)
    LOG(INFO) << __FUNCTION__ << "Connect to redisServer success " << " ip:" << config_.ip << " port:" << config_.port
              << std::endl;

  return ret;
}

int RedisConnector::ExecuteRedisCmd(const char *pszFormat, ...) {
  va_list argList;
  int ret = -1;
  bool reply_status;
  int retry_cnt = 0;

  //连接成功 执行redis 命令
  while (retry_cnt < 3) {
    if (nullptr == redis_) {
      ret = Connect();
      if (ret != 0) {
        retry_cnt++;
        continue;
      }
    }
    va_start(argList, pszFormat);
    reply_ = (redisReply *) redisvCommand(redis_, pszFormat, argList);
    va_end(argList);
    if (nullptr != reply_) {
      reply_status = false;
      switch (reply_->type) {
        case REDIS_REPLY_STATUS:
          if (!strcasecmp(reply_->str, "OK"))
            reply_status = true;
          break;
        case REDIS_REPLY_ERROR:LOG(ERROR) << reply_->str << std::endl;
          break;
        default:reply_status = true;
          break;
      }
      //deal abnormal state
      if (!reply_status) {
        if (strstr(reply_->str, "READONLY You can't write against a read only slave")) {
          return -2;
        } else if (strstr(reply_->str, "NOSCRIPT No matching script. Please use EVAL")) {
          return -3;
        }
      } else {
        ret = 0;
        break;
      }
    } else {
      if (redis_) {
        redisFree(redis_);
        redis_ = nullptr;
      }
      LOG(ERROR) << "redisCommand fail, reply is NULL, try：%d" << retry_cnt << std::endl;
      retry_cnt++;
      continue;
    }
  }

  if (reply_) {
    freeReplyObject(reply_);
    reply_ = nullptr;
  }
  return ret;
}

int RedisConnector::Reconnect() {
  if (redis_) {
    redisFree(redis_);
    redis_ = nullptr;
  }

  return Connect();
}
