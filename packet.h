#ifndef MYSQL2REDIS__PACKET_H_
#define MYSQL2REDIS__PACKET_H_

#include <stdint.h>
#include <string.h>
#include <string>
#include <map>
#include "util.h"
#include "event.h"
#include "config.h"
#include "redis_connector.h"
#include "mysql_type_define.h"
#include "filter.h"

using std::string;
using std::map;

class NormalPacket;

class PacketBase {
 public:
  virtual int Init(uint32_t body_len, uint8_t seq, uint8_t err_flag, char *body_buf);
  virtual int Parse() = 0;

 protected:
  uint32_t body_len_;
  uint8_t seq_;
  uint8_t err_flag_;
  char *body_buf_;
  uint32_t pos_;

};

class ErrorPacket : public PacketBase {
 public:
  ErrorPacket() = default;
  ~ErrorPacket() = default;

  int Parse() override;

 private:
  uint16_t err_code_{};
  char state_[6]{};
  string info_;
};

class NormalPacket : public PacketBase {
 public:
  NormalPacket(std::shared_ptr<PacketStruct> packet)
      : packet_struct_(packet) {};
  ~NormalPacket() {};

  int Parse() override;

  int ParseRotateEvent();

  int ParseFormatEvent();

  int ParseTablemapEvent();

  int ParseRowEvent();

  std::shared_ptr<PacketStruct> packet_struct_;
};

class Packet {
 public:
  Packet() = default;
  ~Packet() = default;

  int Init(bool checksum_enable);

  /**
   * @description: 每次读一个package，根据包长，将数据读入缓冲区m_pBodyBuf，长度存入m_uBodyLen
   * @param {int} fd
   * @return {*} 读取的包长
   */
  int Read(int fd);

  int Parse();

  int ReallocBuf(uint32_t new_size);

 private:
  map<uint8_t, PacketBase *> type_2_packet;
  uint32_t body_len_;
  uint32_t body_buf_len_;
  char *body_buf_;

  const uint32_t head_len_ = 4;
  char head_buf_[4];

  std::shared_ptr<PacketStruct> packet_struct_;
};

#endif //MYSQL2REDIS__PACKET_H_
