#ifndef MYSQL2REDIS__LINE_DATA_H_
#define MYSQL2REDIS__LINE_DATA_H_

#include <vector>
#include <string>
#include <cassert>
#include <algorithm>

#include "config.h"
#include "glog/logging.h"

using std::vector;
using std::string;

enum EnumAction {
  INSERT,
  UPDATE,
  DELETE
};

/*
 *@todo 存储解析后的行数据
 */
class LineData {
 public:
  LineData() = default;;
  ~LineData() = default;;
  int Init(EnumAction enum_action, string database_name, string table_name);

  int PushBack(char *buf, bool is_old);

  int PushBack(string buf, bool is_old);

  int PushBack(char *buf, uint32_t buf_len, bool is_old);

  int ReserveSomeColumn(const std::map<uint32_t, std::string> &remain);

  string GetDatabaseTableName() const { return database_name_ + "_" + table_name_; }

  string &GetColumnByIndex(uint32_t index, bool is_old);

  EnumAction GetLineAction();

  int GetColumnIndexByName(std::string name);

  void SetDiscard(bool is_discard) { is_discard_ = is_discard; }

  bool GetIsDiscard() const { return is_discard_; }

 private:
  bool is_discard_;
  EnumAction action_;
  std::vector<string> column_name_;
  vector<string> new_column_;
  vector<string> old_column_;
  string database_name_;
  string table_name_;
};

#endif //MYSQL2REDIS__LINE_DATA_H_
