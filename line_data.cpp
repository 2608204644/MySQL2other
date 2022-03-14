#include "line_data.h"
int LineData::Init(EnumAction enum_action, string database_name, string table_name) {
  LOG(INFO) << __FUNCTION__ << " get " << enum_action << "_Row_event" << std::endl;
  is_discard_ = false;
  action_ = enum_action;
  database_name_ = database_name;
  table_name_ = table_name;
  return 0;
}

int LineData::PushBack(char *buf, bool is_old) {
  if (is_old) {
    old_column_.emplace_back(buf);
  } else {
    new_column_.emplace_back(buf);
  }
  return 0;
}

int LineData::PushBack(string buf, bool is_old) {
  if (is_old) {
    old_column_.push_back(std::move(buf));
  } else {
    new_column_.push_back(std::move(buf));
  }
  return 0;
}

int LineData::PushBack(char *buf, uint32_t buf_len, bool is_old) {
  std::string tmp(buf, buf_len);
  if (is_old) {
    old_column_.push_back(std::move(tmp));
  } else {
    new_column_.push_back(std::move(tmp));
  }
  return 0;
}

string &LineData::GetColumnByIndex(uint32_t index, bool is_old) {
  if (is_old) {
    assert(index < old_column_.size());
    return old_column_[index];
  } else {
    assert(index < new_column_.size());
    return new_column_[index];
  }
}
int LineData::ReserveSomeColumn(const std::map<uint32_t, std::string> &remain) {
  if (remain.empty())return 0;

  int cnt = std::max(new_column_.size(), old_column_.size());
  auto tmp_old_column = std::move(old_column_);
  auto tmp_new_column = std::move(new_column_);
  old_column_.reserve(remain.size());
  new_column_.reserve(remain.size());
  column_name_.reserve(remain.size());

  for (auto i = remain.begin(); i != remain.end(); ++i) {
    assert(i->first <= cnt);
    column_name_.push_back(i->second);
    if (action_ == DELETE)
      old_column_.push_back(tmp_old_column[i->first - 1]);
    else if (action_ == INSERT)
      new_column_.push_back(tmp_new_column[i->first - 1]);
    else {
      old_column_.push_back(tmp_old_column[i->first - 1]);
      new_column_.push_back(tmp_new_column[i->first - 1]);
    }
  }

  return 0;
}
EnumAction LineData::GetLineAction() {
  return action_;
}
int LineData::GetColumnIndexByName(std::string name) {
  int n = column_name_.size();
  for (int i = 0; i < n; ++i) {
    if (!column_name_[i].compare(name)) return i;
  }
  return -1;
}

