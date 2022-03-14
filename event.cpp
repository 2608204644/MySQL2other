//
// Created by cha on 2022/3/10.
//

#include "event.h"
#include "event.h"

int EventCommHead::Parse(char *buf, uint32_t &pos) {
  time_stamp_ = uint4korr(buf + pos);
  pos += 4;
  event_type_ = buf[pos];
  pos += 1;
  server_id_ = uint4korr(buf + pos);
  pos += 4;
  event_size_ = uint4korr(buf + pos);
  pos += 4;
  log_pos_ = uint4korr(buf + pos);
  pos += 4;
  head_flags_ = uint2korr(buf + pos);
  pos += 2;

  return 0;
}

int RotateEvent::Parse(char *buf, uint32_t &pos, uint32_t buf_len) {
  next_binlog_pos_ = uint8korr(buf + pos);
  pos += 8;

  int uLeft = buf_len - pos;
  next_binlog_filename_.assign(buf + pos, uLeft);;
  pos += uLeft;

  return 0;
}

int FormatEvent::Parse(char *buf, uint32_t &pos, uint32_t buf_len, bool checksum_enable) {
  binlog_ver_ = uint2korr(buf + pos);
  pos += 2;

  memset(server_ver_, 0, sizeof(server_ver_));
  strncpy(server_ver_, buf + pos, 50);
  pos += 50;

  format_time_stamp_ = uint4korr(buf + pos);
  pos += 4;

  head_len_ = buf[pos];
  pos += 1;

  uint32_t uPostHeadLenStrLen = 0;
  if (checksum_enable == true) {
    uPostHeadLenStrLen = buf_len - pos - BINLOG_CHECKSUM_LEN - BINLOG_CHECKSUM_ALG_DESC_LEN;
    post_head_len_str_.assign(buf + pos, uPostHeadLenStrLen);
    pos += uPostHeadLenStrLen;

    alg_ = buf[pos];
    pos += 1;
  } else {
    uPostHeadLenStrLen = buf_len - pos;
    post_head_len_str_.assign(buf + pos, uPostHeadLenStrLen);
  }

  return 0;
}

uint8_t FormatEvent::GetPostHeadLen(uint32_t type) {
  assert(type <= post_head_len_str_.size());
  return post_head_len_str_[type - 1];
}

uint64_t TablemapEvent::GetTableid(char *buf, uint32_t pos, FormatEvent *format_event) {
  uint64_t table_id;
  uint8_t u8PostHeadLen = format_event->GetPostHeadLen(0x13);

  if (u8PostHeadLen == 6) {
    table_id = uint4korr(buf + pos);
  } else {
    table_id = uint6korr(buf + pos);
  }

  return table_id;
}

int TablemapEvent::Parse(char *buf, uint32_t &pos, uint32_t buf_len, FormatEvent *format_event) {
  uint8_t post_head_len = format_event->GetPostHeadLen(cmd_);
  //post header
  if (post_head_len == 6) {
    table_id_ = uint4korr(buf + pos);
    pos += 4;
  } else {
    table_id_ = uint6korr(buf + pos);
    pos += 6;
  }

  flags_ = uint2korr(buf + pos);
  pos += 2;

  //payload
  database_name_len_ = buf[pos];
  pos += 1;
  database_name_.assign(buf + pos, database_name_len_);
  pos += database_name_len_;
  pos += 1;

  table_name_len_ = buf[pos];
  pos += 1;
  table_name_.assign(buf + pos, table_name_len_);
  pos += table_name_len_;
  pos += 1;

  UnpackLenencInt(buf + pos, pos, column_count_);

  column_type_def_.Assign(buf + pos, column_count_);
  pos += column_count_;

  UnpackLenencInt(buf + pos, pos, column_meta_len_);
  column_meta_def_.Assign(buf + pos, column_meta_len_);
  pos += column_meta_len_;

  uint64_t null_bitmap_len = (column_count_ + 7) / 8;
  null_bit_.Assign(buf + pos, null_bitmap_len);
  pos += null_bitmap_len;


  // metadata
  uint32_t meta_pos = 0;
  uint16_t Meta = 0;
  for (uint32_t i = 0; i < column_count_; i++) {
    Meta = ReadMeta(column_meta_def_, meta_pos, column_type_def_.At(i));
    metas_.push_back(Meta);
  }
  return 0;
}

uint16_t TablemapEvent::ReadMeta(MemBlock &column_meta_def, uint32_t &meta_pos, uint8_t column_type) {
  uint16_t u16Meta = 0;
  if (column_type == MYSQL_TYPE_TINY_BLOB ||
      column_type == MYSQL_TYPE_BLOB ||
      column_type == MYSQL_TYPE_MEDIUM_BLOB ||
      column_type == MYSQL_TYPE_LONG_BLOB ||
      column_type == MYSQL_TYPE_DOUBLE ||
      column_type == MYSQL_TYPE_FLOAT ||
      column_type == MYSQL_TYPE_GEOMETRY ||
      column_type == MYSQL_TYPE_TIME2 ||
      column_type == MYSQL_TYPE_DATETIME2 ||
      column_type == MYSQL_TYPE_TIMESTAMP2) {
    u16Meta = (uint16_t) (uint8_t) column_meta_def.At(meta_pos);
    meta_pos += 1;
  } else if (column_type == MYSQL_TYPE_NEWDECIMAL ||
      column_type == MYSQL_TYPE_VAR_STRING ||
      column_type == MYSQL_TYPE_STRING ||
      column_type == MYSQL_TYPE_SET ||
      column_type == MYSQL_TYPE_ENUM) {
    u16Meta = ((uint16_t) (uint8_t) column_meta_def.At(meta_pos) << 8)
        | ((uint16_t) (uint8_t) column_meta_def.At(meta_pos + 1));
    meta_pos += 2;
  } else if (column_type == MYSQL_TYPE_VARCHAR) {
    u16Meta = uint2korr(column_meta_def.m_pBlock + meta_pos);
    meta_pos += 2;
  } else if (column_type == MYSQL_TYPE_BIT) {
    u16Meta = ((uint16_t) (uint8_t) column_meta_def.At(meta_pos) << 8)
        | ((uint16_t) (uint8_t) column_meta_def.At(meta_pos + 1));
    meta_pos += 2;

  } else if (column_type == MYSQL_TYPE_DECIMAL ||
      column_type == MYSQL_TYPE_TINY ||
      column_type == MYSQL_TYPE_SHORT ||
      column_type == MYSQL_TYPE_LONG ||
      column_type == MYSQL_TYPE_NULL ||
      column_type == MYSQL_TYPE_TIMESTAMP ||
      column_type == MYSQL_TYPE_LONGLONG ||
      column_type == MYSQL_TYPE_INT24 ||
      column_type == MYSQL_TYPE_DATE ||
      column_type == MYSQL_TYPE_TIME ||
      column_type == MYSQL_TYPE_DATETIME ||
      column_type == MYSQL_TYPE_YEAR) {
    u16Meta = 0;
  } else {
    LOG(ERROR) << "TablemapEvent::ReadMeta column type can not exist in the binlog, columntype" << column_type
               << std::endl;
  }

  return u16Meta;
}

uint8_t TablemapEvent::GetColumnType(uint32_t column_index) {
  return column_type_def_.At(column_index);
}

uint16_t TablemapEvent::GetColumnMeta(uint32_t column_index) {
  return metas_[column_index];
}

/***********************class RowEvent*******************/
int RowEvent::Init(uint8_t cmd) {
  row_event_cmd_ = cmd;
  if (cmd == 0x17 || cmd == 0x18 || cmd == 0x19)
    row_event_ver_ = 1;
  else if (cmd == 0x1E || cmd == 0x1F || cmd == 0x20)
    row_event_ver_ = 2;

  return 0;
}

uint64_t RowEvent::GetTableid(char *buf, uint32_t pos, uint8_t cmd, FormatEvent *format_event) {
  uint64_t tableid = 0;
  uint8_t head_len = format_event->GetPostHeadLen(cmd);

  if (head_len == 6) {
    tableid = uint4korr(buf + pos);
    pos += 4;
  } else {
    tableid = uint6korr(buf + pos);
    pos += 6;
  }

  return tableid;
}

int RowEvent::Parse(char *buf,
                    uint32_t &pos,
                    uint32_t buf_len,
                    FormatEvent *format_event,
                    TablemapEvent *tablemap_event,
                    std::shared_ptr<PacketStruct> packet_struct) {
  uint8_t post_head_len = format_event->GetPostHeadLen(row_event_cmd_);
  if (post_head_len == 6) {
    table_id_ = uint4korr(buf + pos);
    pos += 4;
  } else {
    table_id_ = uint6korr(buf + pos);
    pos += 6;
  }
  row_event_flag_ = uint2korr(buf + pos);
  pos += 2;

  if (row_event_ver_ == 2) {
    extra_data_length_ = uint2korr(buf + pos);
    pos += 2;
    uint16_t extra_data_length = extra_data_length_ - 2;
    if (extra_data_length > 0) {
      extra_data_.assign(buf + pos, extra_data_length);
      pos += extra_data_length;
    }
  }

  //payload
  UnpackLenencInt(buf + pos, pos, column_count_);

  uint32_t bitmap_len = (column_count_ + 7) / 8;
  present_bitmap_1_.Assign(buf + pos, bitmap_len);
  pos += bitmap_len;

  uint32_t bitset_count = present_bitmap_1_.GetBitsetCount(column_count_);
  uint32_t null_bitmap_1_size = (bitset_count + 7) / 8;

  uint32_t uNullBitmap2Size = 0;
  if (row_event_cmd_ == 0x18 || row_event_cmd_ == 0x1F) { //update v1,v2 code
    present_bitmap_2_.Assign(buf + pos, bitmap_len);
    pos += bitmap_len;

    uint32_t uPresentBitmapBitset2 = present_bitmap_2_.GetBitsetCount(column_count_);
    uNullBitmap2Size = (uPresentBitmapBitset2 + 7) / 8;
  }

  //rows
  while (buf_len > pos) {
    auto line_data = std::make_shared<LineData>();
    if (line_data == nullptr) {
      LOG(ERROR) << __FUNCTION__ << " new Row() error" << std::endl;
      return -1;
    }

    if (row_event_cmd_ == 0x17 || row_event_cmd_ == 0x1E) { //insert event
      line_data->Init(INSERT, tablemap_event->database_name_, tablemap_event->table_name_);
      ParseRow(buf, pos, null_bitmap_1_size, line_data, &present_bitmap_1_, false, tablemap_event);
    } else if (row_event_cmd_ == 0x19 || row_event_cmd_ == 0x20) {
      line_data->Init(DELETE, tablemap_event->database_name_, tablemap_event->database_name_);
      ParseRow(buf, pos, null_bitmap_1_size, line_data, &present_bitmap_1_, false, tablemap_event);
    } else if (row_event_cmd_ == 0x18 || row_event_cmd_ == 0x1F) {
      line_data->Init(UPDATE, tablemap_event->database_name_, tablemap_event->table_name_);
      ParseRow(buf, pos, null_bitmap_1_size, line_data, &present_bitmap_1_, true, tablemap_event);
      ParseRow(buf, pos, uNullBitmap2Size, line_data, &present_bitmap_2_, false, tablemap_event);
    } else {
      LOG(ERROR) << __FUNCTION__ << " unsport commond: " << row_event_cmd_ << std::endl;
      return -1;
    }

    Filter::GetFilterInstance().FilterData(line_data);
    if (!line_data->GetIsDiscard()) {
      EventDispatcher::GetEventDispatcherInstance().PutLineData(line_data);
    }

  }

  return 0;
}

int RowEvent::ParseRow(char *buf,
                       uint32_t &pos,
                       uint32_t null_bitmap_size,
                       std::shared_ptr<LineData> line_data,
                       MemBlock *present_bitmap,
                       bool is_old,
                       TablemapEvent *tablemap_event) {
  int ret = 0;
  bool an_unsigned = false;

  null_bitmap_.Assign(buf + pos, null_bitmap_size);
  pos += null_bitmap_size;

  uint32_t present_pos = 0;
  for (uint32_t i = 0; i < column_count_; i++) {
    if (present_bitmap->GetBit(i)) {
      bool bNullCol = null_bitmap_.GetBit(present_pos);
      if (bNullCol) {
        line_data->PushBack(string(""), is_old);
      } else {
        uint8_t u8ColType = tablemap_event->GetColumnType(i);
        uint16_t u16Meta = tablemap_event->GetColumnMeta(i);
        ret = ParseColumnValue(buf + pos, pos, u8ColType, u16Meta, is_old, an_unsigned, line_data);
        if (ret == -1)
          return -1;
      }

      present_pos++;
    } else {
      line_data->PushBack(string(""), is_old);
    }

  }

  return 0;
}

int RowEvent::ParseColumnValue(char *buf,
                               uint32_t &pos,
                               uint8_t col_type,
                               uint16_t meta,
                               bool is_old,
                               bool an_unsigned,
                               std::shared_ptr<LineData> line_data) {
  uint32_t uLength = 0;
  if (col_type == MYSQL_TYPE_STRING) {
    if (meta >= 256) {
      uint8_t byte_0 = meta >> 8;
      uint8_t byte_1 = meta & 0xFF;
      if ((byte_0 & 0x30) != 0x30) {
        uLength = byte_1 | (((byte_0 & 0x30) ^ 0x30) << 4);
        col_type = byte_0 | 0x30;
      } else {
        switch (byte_0) {
          case MYSQL_TYPE_SET:
          case MYSQL_TYPE_ENUM:
          case MYSQL_TYPE_STRING:col_type = byte_0;
            uLength = byte_1;
            break;
          default:LOG(ERROR) << " don't know how to handle column type:" << col_type << " meta: " << meta << std::endl;
            return -1;
        }
      }
    } else {
      uLength = meta;
    }
  }

  char szValue[256] = {0};
  switch (col_type) {
    case MYSQL_TYPE_LONG://int32
    {
      if (an_unsigned) {
        uint32_t i = uint4korr(buf);
        snprintf(szValue, sizeof(szValue), "%u", i);
      } else {
        int32_t i = sint4korr(buf);
        snprintf(szValue, sizeof(szValue), "%d", i);
      }
      pos += 4;
      line_data->PushBack(szValue, is_old);
      break;
    }
    case MYSQL_TYPE_TINY: {
      if (an_unsigned) {
        uint8_t c = (uint8_t) (*buf);
        snprintf(szValue, sizeof(szValue), "%hhu", c);
      } else {
        int8_t c = (int8_t) (*buf);
        snprintf(szValue, sizeof(szValue), "%hhd", c);
      }
      pos += 1;
      line_data->PushBack(szValue, is_old);
      break;
    }
    case MYSQL_TYPE_SHORT: {
      if (an_unsigned) {
        uint16_t s = uint2korr(buf);
        snprintf(szValue, sizeof(szValue), "%hu", s);
      } else {
        int16_t s = sint2korr(buf);
        snprintf(szValue, sizeof(szValue), "%hd", s);
      }
      pos += 2;
      line_data->PushBack(szValue, is_old);
      break;
    }
    case MYSQL_TYPE_LONGLONG: {
      if (an_unsigned) {
        uint64_t ll = uint8korr(buf);
        snprintf(szValue, sizeof(szValue), "%lu", ll);
      } else {
        int64_t ll = sint8korr(buf);
        snprintf(szValue, sizeof(szValue), "%ld", ll);
      }
      pos += 8;
      line_data->PushBack(szValue, is_old);
      break;
    }
    case MYSQL_TYPE_INT24: {
      int32_t i;
      if (an_unsigned) {
        i = uint3korr(buf);
      } else {
        i = sint3korr(buf);
      }
      snprintf(szValue, sizeof(szValue), "%d", i);
      pos += 3;
      line_data->PushBack(szValue, is_old);
      break;
    }
    case MYSQL_TYPE_TIMESTAMP: {
      uint32_t i = uint4korr(buf);
      snprintf(szValue, sizeof(szValue), "%u", i);
      pos += 4;
      line_data->PushBack(szValue, is_old);
      break;
    }
    case MYSQL_TYPE_DATETIME: {
      uint64_t ll = uint8korr(buf);
      uint32_t d = ll / 1000000;
      uint32_t t = ll % 1000000;
      snprintf(szValue, sizeof(szValue),
               "%04d-%02d-%02d %02d:%02d:%02d",
               d / 10000, (d % 10000) / 100, d / 100,
               t / 10000, (t % 10000) / 100, t / 100);
      pos += 8;
      line_data->PushBack(szValue, is_old);
      break;
    }
    case MYSQL_TYPE_TIME: {
      uint32_t i32 = uint3korr(buf);
      snprintf(szValue, sizeof(szValue), "%02d:%02d:%02d", i32 / 10000, (i32 % 10000) / 100, i32 / 100);
      pos += 3;
      line_data->PushBack(szValue, is_old);
      break;
    }
    case MYSQL_TYPE_NEWDATE: {
      uint32_t tmp = uint3korr(buf);
      int part;
      char sbuf[11];
      char *spos = &sbuf[10];  // start from '\0' to the beginning

      /* Copied from field.cc */
      *spos-- = 0;                 // End NULL
      part = (int) (tmp & 31);
      *spos-- = (char) ('0' + part % 10);
      *spos-- = (char) ('0' + part / 10);
      *spos-- = ':';
      part = (int) (tmp >> 5 & 15);
      *spos-- = (char) ('0' + part % 10);
      *spos-- = (char) ('0' + part / 10);
      *spos-- = ':';
      part = (int) (tmp >> 9);
      *spos-- = (char) ('0' + part % 10);
      part /= 10;
      *spos-- = (char) ('0' + part % 10);
      part /= 10;
      *spos-- = (char) ('0' + part % 10);
      part /= 10;
      *spos = (char) ('0' + part);

      snprintf(szValue, sizeof(szValue), "%s", sbuf);
      pos += 3;
      line_data->PushBack(szValue, is_old);
      break;
    }
    case MYSQL_TYPE_DATE: {
      uint32_t i32 = uint3korr(buf);
      snprintf(szValue,
               sizeof(szValue),
               "%04d-%02d-%02d",
               (int32_t) (i32 / (16L * 32L)),
               (int32_t) (i32 / 32L % 16L),
               (int32_t) (i32 % 32L));
      pos += 3;
      line_data->PushBack(szValue, is_old);
      break;
    }
    case MYSQL_TYPE_YEAR: {
      uint32_t i = (uint32_t) (uint8_t) *buf;
      snprintf(szValue, sizeof(szValue), "%04d", i + 1900);
      pos += 1;
      line_data->PushBack(szValue, is_old);
      break;
    }
    case MYSQL_TYPE_ENUM: {
      switch (uLength) {
        case 1: {
          snprintf(szValue, sizeof(szValue), "%d", (int32_t) *buf);
          pos += 1;
          line_data->PushBack(szValue, is_old);
          break;
        }
        case 2: {
          int32_t i32 = uint2korr(buf);
          snprintf(szValue, sizeof(szValue), "%d", i32);
          pos += 2;
          line_data->PushBack(szValue, is_old);
          break;
        }
        default:LOG(ERROR) << __FUNCTION__ << " unknown enum packlen:" << uLength << std::endl;
          return -1;
      }
      break;
    }
    case MYSQL_TYPE_SET: {
      pos += uLength;
      line_data->PushBack(buf, uLength, is_old);
      break;
    }
    case MYSQL_TYPE_BLOB: {
      switch (meta) {
        case 1:     //TINYBLOB/TINYTEXT
        {
          uLength = (uint8_t) (*buf);
          pos += uLength + 1;
          line_data->PushBack(buf + 1, uLength, is_old);
          break;
        }
        case 2:     //BLOB/TEXT
        {
          uLength = uint2korr(buf);
          pos += uLength + 2;
          line_data->PushBack(buf + 2, uLength, is_old);
          break;
        }
        case 3:     //MEDIUMBLOB/MEDIUMTEXT
        {
          uLength = uint3korr(buf);
          pos += uLength + 3;
          line_data->PushBack(buf + 3, uLength, is_old);
          break;
        }
        case 4:     //LONGBLOB/LONGTEXT
        {
          uLength = uint4korr(buf);
          pos += uLength + 4;
          line_data->PushBack(buf + 4, uLength, is_old);
          break;
        }
        default:LOG(ERROR) << __FUNCTION__ << " unknown blob type" << meta << std::endl;
          return -1;
      }
      break;
    }
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING: {
      uLength = meta;
      if (uLength < 256) {
        uLength = (uint8_t) (*buf);
        pos += 1 + uLength;
        line_data->PushBack(buf + 1, uLength, is_old);
      } else {
        uLength = uint2korr(buf);
        pos += 2 + uLength;
        line_data->PushBack(buf + 2, uLength, is_old);
      }
      break;
    }
    case MYSQL_TYPE_STRING: {
      if (uLength < 256) {
        uLength = (uint8_t) (*buf);
        pos += 1 + uLength;
        line_data->PushBack(buf + 1, uLength, is_old);
      } else {
        uLength = uint2korr(buf);
        pos += 2 + uLength;
        line_data->PushBack(buf + 2, uLength, is_old);
      }
      break;
    }
    case MYSQL_TYPE_BIT: {
      uint32_t nBits = (meta >> 8) + (meta & 0xFF) * 8;
      uLength = (nBits + 7) / 8;
      pos += uLength;
      line_data->PushBack(buf, uLength, is_old);
      break;
    }
    case MYSQL_TYPE_FLOAT: {
      float fl;
      float4get(fl, buf);
      pos += 4;
      snprintf(szValue, sizeof(szValue), "%f", (double) fl);
      line_data->PushBack(szValue, is_old);
      break;
    }
    case MYSQL_TYPE_DOUBLE: {
      double dbl;
      float8get(dbl, buf);
      pos += 8;
      snprintf(szValue, sizeof(szValue), "%f", dbl);
      line_data->PushBack(szValue, is_old);
      break;
    }
    case MYSQL_TYPE_NEWDECIMAL: {
      uint8_t precision = meta >> 8;
      uint8_t decimals = meta & 0xFF;
      int bin_size = decimal_get_binary_size(precision, decimals);
      my_decimal dec;
      binary2my_decimal(E_DEC_FATAL_ERROR, (unsigned char *) buf, &dec, precision, decimals);
      int i, end;
      char buff[512], *spos;
      spos = buff;
      spos += sprintf(buff, "%s", dec.sign() ? "-" : "");
      end = ROUND_UP(dec.frac) + ROUND_UP(dec.intg) - 1;
      for (i = 0; i < end; i++)
        spos += sprintf(spos, "%09d.", dec.buf[i]);
      spos += sprintf(spos, "%09d", dec.buf[i]);
      line_data->PushBack(buff, is_old);
      pos += bin_size;
    }
    case MYSQL_TYPE_TIMESTAMP2: {
      struct timeval tm;
      parse_timestamp_from_binary(&tm, buf, pos, meta);
      snprintf(szValue, sizeof(szValue), "%u.%u", (uint32_t) tm.tv_sec, (uint32_t) tm.tv_usec);
      line_data->PushBack(szValue, is_old);
      break;
    }
    case MYSQL_TYPE_DATETIME2: {
      int64_t ymd, hms;
      int64_t ymdhms, ym;
      int frac;
      int64_t packed = parse_datetime_packed_from_binary(buf, pos, meta);
      if (packed < 0)
        packed = -packed;

      ymdhms = PARSE_PACKED_TIME_GET_INT_PART(packed);
      frac = PARSE_PACKED_TIME_GET_FRAC_PART(packed);

      ymd = ymdhms >> 17;
      ym = ymd >> 5;
      hms = ymdhms % (1 << 17);

      int day = ymd % (1 << 5);
      int month = ym % 13;
      int year = ym / 13;

      int second = hms % (1 << 6);
      int minute = (hms >> 6) % (1 << 6);
      int hour = (hms >> 12);

      snprintf(szValue,
               sizeof(szValue),
               "%04d-%02d-%02d %02d:%02d:%02d.%d",
               year,
               month,
               day,
               hour,
               minute,
               second,
               frac);
      line_data->PushBack(szValue, is_old);
      break;
    }
    case MYSQL_TYPE_TIME2: {
      assert(meta <= DATETIME_MAX_DECIMALS);
      int64_t packed = parse_time_packed_from_binary(buf, pos, meta);
      if (packed < 0)
        packed = -packed;

      long hms = PARSE_PACKED_TIME_GET_INT_PART(packed);
      int frac = PARSE_PACKED_TIME_GET_FRAC_PART(packed);

      int hour = (hms >> 12) % (1 << 10);
      int minute = (hms >> 6) % (1 << 6);
      int second = hms % (1 << 6);
      snprintf(szValue, sizeof(szValue), "%02d:%02d:%02d.%d", hour, minute, second, frac);
      line_data->PushBack(szValue, is_old);
      break;
    }

    default:LOG(ERROR) << __FUNCTION__ << " don't know how to handle type =" << col_type << " meta=" << meta
                       << " column value" << std::endl;
      return -1;

  }

  return 0;
}
