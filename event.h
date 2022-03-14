//
// Created by cha on 2022/3/10.
//

#ifndef MYSQL2REDIS__EVENT_H_
#define MYSQL2REDIS__EVENT_H_

#include <string>
#include <vector>
#include <map>
#include "util.h"
#include "glog/logging.h"
#include "line_data.h"
#include "my_decimal.h"
#include "config.h"
#include "mysql_type_define.h"
#include "memblock.h"
#include "filter.h"
#include "reserve_data.h"
#include "event_dispatcher.h"

using std::string;
using std::vector;
using std::map;

class EventCommHead;
class RotateEvent;
class FormatEvent;
class TablemapEvent;
class RowEvent;
struct PacketStruct;

class EventCommHead {

 public:
  EventCommHead() {};
  ~EventCommHead() {};

  int Parse(char *buf, uint32_t &pos);

 public:
  uint32_t time_stamp_;
  uint8_t event_type_;
  uint32_t server_id_;
  uint32_t event_size_;
  uint32_t log_pos_;
  uint16_t head_flags_;
};

class RotateEvent {

 public:
  RotateEvent() {};
  ~RotateEvent() {};

  int Parse(char *buf, uint32_t &pos, uint32_t buf_len);

 public:
  uint32_t next_binlog_pos_;
  string next_binlog_filename_;
};

class FormatEvent {
 public:
  FormatEvent() : alg_(0) {};
  ~FormatEvent() {};

  int Parse(char *buf, uint32_t &pos, uint32_t buf_len, bool checksum_enable);
  uint8_t GetPostHeadLen(uint32_t type);

 public:
  uint16_t binlog_ver_;
  char server_ver_[51];
  uint32_t format_time_stamp_;
  uint8_t head_len_;
  string post_head_len_str_;
  uint8_t alg_;
};

class TablemapEvent {

 public:
  TablemapEvent() : cmd_(0x13) {};
  ~TablemapEvent() {};

  static uint64_t GetTableid(char *buf, uint32_t pos, FormatEvent *format_event);
  int Parse(char *buf, uint32_t &pos, uint32_t buf_len, FormatEvent *format_event);

  uint16_t ReadMeta(MemBlock &column_meta_def, uint32_t &meta_pos, uint8_t column_type);
  uint8_t GetColumnType(uint32_t column_index);
  uint16_t GetColumnMeta(uint32_t column_index);

 public:
  uint8_t cmd_;
  uint64_t table_id_;
  uint16_t flags_;
  uint8_t database_name_len_;
  string database_name_;
  uint8_t table_name_len_;
  string table_name_;
  uint64_t column_count_;
  MemBlock column_type_def_;
  uint64_t column_meta_len_;
  MemBlock column_meta_def_;
  MemBlock null_bit_;

  vector<uint16_t> metas_;
};

class RowEvent {

 public:
  RowEvent() = default;;
  ~RowEvent() = default;;

  int Init(uint8_t cmd);
  int Parse(char *buf,
            uint32_t &pos,
            uint32_t buf_len,
            FormatEvent *format_event,
            TablemapEvent *tablemap_event,
            std::shared_ptr<PacketStruct> packet_struct);
  int ParseRow(char *buf,
               uint32_t &pos,
               uint32_t null_bitmap_size,
               std::shared_ptr<LineData> line_data,
               MemBlock *present_bitmap,
               bool is_old,
               TablemapEvent *tablemap_event);
  int ParseColumnValue(char *buf,
                       uint32_t &pos,
                       uint8_t col_type,
                       uint16_t meta,
                       bool is_old,
                       bool an_unsigned,
                       std::shared_ptr<LineData> line_data);

  static uint64_t GetTableid(char *buf, uint32_t pos, uint8_t cmd, FormatEvent *format_event);

 public:
  uint8_t row_event_ver_{};
  uint8_t row_event_cmd_{};
  uint64_t table_id_{};
  uint16_t row_event_flag_{};
  uint16_t extra_data_length_{};
  string extra_data_;
  uint64_t column_count_{};
  MemBlock present_bitmap_1_;
  MemBlock present_bitmap_2_;
  MemBlock null_bitmap_;
};

struct PacketStruct {
  EventCommHead event_comm_head;
  RotateEvent rotate_event;
  FormatEvent format_event;
  map<uint64_t, TablemapEvent *> table_id2tablemap_event;
  RowEvent row_event;

  string next_binlog_file_name;
  uint32_t next_binlog_pos;
  bool checksum_enable;
};

#endif //MYSQL2REDIS__EVENT_H_
