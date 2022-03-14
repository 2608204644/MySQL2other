#include "mysql_connector.h"

MysqlConnector::MysqlConnector() = default;

MysqlConnector::~MysqlConnector() = default;

int MysqlConnector::init(const MysqlConnectConfig &config) {

  connect_config.ip = config.ip;
  connect_config.port = config.port;
  connect_config.username = config.username;
  connect_config.password = config.password;
  connect_config.need_decode = config.need_decode;
  connect_config.server_id = config.server_id;

  //decode password
  if (connect_config.need_decode != 0) {
    int len = 64;
    unsigned char swap[64] = {0};
    unsigned char password_decode[512] = {0};
    Base64Decode((unsigned char *) connect_config.password.c_str(), connect_config.password.size(), swap, &len);
    uint32_t decode_len = 0;
    if (!DesDecrypt(swap, len, password_decode, decode_len, (unsigned char *) "WorkECJol")) {
      LOG(ERROR) << __FUNCTION__ << " decrypt mysql password error " << connect_config.password << std::endl;
      return -1;
    }
    connect_config.password.assign(reinterpret_cast<const char *>(password_decode));
  }
}

int MysqlConnector::Connect() {

  if (mysql_init(&mysql) == NULL) {
    LOG(ERROR) << __FUNCTION__ << " failed to init mysql connection" << std::endl;
    return -1;
  }
  char value = 1;
  mysql_options(&mysql, MYSQL_OPT_RECONNECT, &value);

  if (mysql_real_connect(&mysql,
                         connect_config.ip.c_str(),
                         connect_config.username.c_str(),
                         connect_config.password.c_str(),
                         nullptr,
                         connect_config.port,
                         nullptr,
                         0)
      == nullptr) {
    LOG(ERROR) << __FUNCTION__ << " failed to connect mysql master,ip: " << connect_config.ip.c_str() << "err: "
               << mysql_error(&mysql) << std::endl;
    return -1;
  }

  LOG(ERROR) << __FUNCTION__ << " connected mysql succeed. fd:" << mysql.net.fd << std::endl;
  connected = true;
  return 0;
}

int MysqlConnector::DisConnect() {
  connected = false;
  mysql_close(&mysql);
  return 0;
}

int MysqlConnector::KeepAlive() {
  if (connected) {
    time_t tNow = time(0);
    if (pre_keep_alive_time + 10 < tNow) {
      mysql_ping(&mysql);
      pre_keep_alive_time = tNow;
    }
  } else {
    Connect();
  }

  return 0;
}

bool MysqlConnector::CheckIsChecksumEnable() {
  mysql_query(&mysql, "SHOW GLOBAL VARIABLES LIKE 'BINLOG_CHECKSUM'");
  MYSQL_RES *result = mysql_store_result(&mysql);
  if (result == nullptr) {
    LOG(ERROR) << __FUNCTION__ << " Failed to get binlog checksum enable Error: " << mysql_error(&mysql) << std::endl;
    return false;
  }
  MYSQL_ROW row = mysql_fetch_row(result);
  if (row == nullptr) {
    DLOG(ERROR) << __FUNCTION__ << " mysql_fetch_row get empty row" << std::endl;
    mysql_free_result(result);
    return false;
  }
  if (strcmp(row[1], "NONE") == 0) {
    mysql_free_result(result);
    return false;
  }

  mysql_free_result(result);
  return true;
}

int MysqlConnector::GetNowBinlogPos(string &binlog_file_name, uint32_t &binlog_pos) {
  mysql_query(&mysql, "show master status");
  MYSQL_RES *result = mysql_store_result(&mysql);
  if (result == nullptr) {
    LOG(ERROR) << __FUNCTION__ << "Failed to get mysql status Error: %s" << mysql_error(&mysql) << std::endl;
    return -1;
  }
  MYSQL_ROW row = mysql_fetch_row(result);
  if (row == nullptr) {
    LOG(ERROR) << __FUNCTION__ << " mysql_fetch_row get empty row" << std::endl << std::endl;
    mysql_free_result(result);
    return -1;
  }

  binlog_file_name.assign(row[0]);
  binlog_pos = atol(row[1]);

  mysql_free_result(result);
  return 0;
}

int MysqlConnector::GetFd() const {
  return mysql.net.fd;
}

int MysqlConnector::ReqBinlog(string &binlog_file_name, uint32_t binlog_pos, bool checksum_enable) {
  int ret;
  int fd;
  int flags;
  fd = mysql.net.fd;

  if (checksum_enable) {
    mysql_query(&mysql, "SET @master_binlog_checksum='NONE'");
  }

  if ((flags = fcntl(fd, F_GETFL)) == -1) {
    LOG(ERROR) << __FUNCTION__ << "fcntl(F_GETFL): " << strerror(errno) << std::endl;
    return -1;
  }

  flags &= ~O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) == -1) {
    LOG(ERROR) << __FUNCTION__ << "[fcntl(F_SETFL,O_BLOCK): " << strerror(errno) << std::endl;
    return -1;
  }

  /*
   *@todo 组包发送到 mysql，请求binlog
   *包结构为  length      3
   *          seq         1
   *          cmd_         1
   *          binlogpos   4
   *          flags       2
   *          serverid    4
   *          binlogfilename    总长度-前面的定长
   */
  char buf[1024] = {0};
  uint32_t body_len = 0;
  uint32_t offset = 4;
  uint8_t cmd = 0x12;
  uint16_t flag = 0x00;
  uint32_t server_id = connect_config.server_id;

  buf[offset] = cmd;
  body_len += 1;
  int4store(buf + offset + body_len, binlog_pos);
  body_len += 4;

  int4store(buf + offset + body_len, flag);
  body_len += 2;

  int4store(buf + offset + body_len, server_id);
  body_len += 4;

  uint32_t uFileNameLen = binlog_file_name.size();
  memcpy(buf + offset + body_len, binlog_file_name.c_str(), uFileNameLen);
  body_len += uFileNameLen;

  buf[offset - 1] = 0x00; //seq
  int3store(buf, body_len);

  ret = write(mysql.net.fd, buf, body_len + 4);
  if (ret != int(body_len + 4)) {
    LOG(ERROR) << __FUNCTION__ << " write dump cmd_ packet fail, error:" << strerror(errno) << std::endl;
    return -1;
  }

  LOG(ERROR) << __FUNCTION__ << " request binlog, file:" << binlog_file_name.c_str() << " , logpos: " << binlog_pos
             << std::endl;

  return 0;
}
