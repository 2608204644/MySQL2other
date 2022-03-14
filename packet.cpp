#include "packet.h"

/*
 * PacketBase
 * */

int PacketBase::Init(uint32_t body_len, uint8_t seq, uint8_t err_flag, char *body_buf) {
  body_len_ = body_len;
  seq_ = seq;
  err_flag_ = err_flag;
  body_buf_ = body_buf;
  pos_ = 1;
  return 0;
}

/*
 * ErrorPacket
 * */

int ErrorPacket::Parse() {
  err_code_ = uint2korr(body_buf_ + pos_);
  pos_ += 2;

  memcpy(state_, body_buf_ + pos_, sizeof(state_));
  pos_ += sizeof(state_);

  uint32_t info_len = body_len_ - pos_;
  info_.assign(*body_buf_ + pos_, info_len);
  pos_ += info_len;

  LOG(ERROR) << "ErrorPackage::Parse recv error packet.errorcode: " << err_code_ << " state: " << state_ << " errinfo: "
             << info_.c_str() << std::endl;

  return 0;
}

/*
 * NormalPacket
 * */

int NormalPacket::Parse() {
  if (body_len_ < 19) {
    LOG(ERROR) << "NormalPacket::Parse packet body length < 19" << std::endl;
    return -1;
  }

  packet_struct_->event_comm_head.Parse(body_buf_, pos_);

  /*
   * If there are no rotate event and format event in the package,  the log_pos_ equal 0
   */
  if (packet_struct_->event_comm_head.log_pos_ != 0) {
    packet_struct_->next_binlog_file_name = packet_struct_->rotate_event.next_binlog_filename_;
    packet_struct_->next_binlog_pos = packet_struct_->event_comm_head.log_pos_;
  }

  switch (packet_struct_->event_comm_head.event_type_) {
    case 0x00: {
      LOG(ERROR) << " event_type:0, unknow event" << std::endl;
      return -1;
    }
    case 0x04: {
      ParseRotateEvent();
      break;
    }
    case 0x0f: {
      ParseFormatEvent();
      break;
    }
    case 0x13: {
      ParseTablemapEvent();
      break;
    }
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1E:
    case 0x1F:
    case 0x20: {
      ParseRowEvent();
      break;
    }
    default: {
      //DOLOG("[trace]%s no process eventType. eventType:0x%X", __FUNCTION__, m_pContext->cEventCommHead.m_u8EventType);
      break;
    }
  }
  return 0;
}

int NormalPacket::ParseRotateEvent() {
  int ret = 0;
  uint32_t body_len_without_crc = body_len_;
  if (packet_struct_->checksum_enable == true && packet_struct_->format_event.alg_ != BINLOG_CHECKSUM_ALG_OFF
      && packet_struct_->format_event.alg_ != BINLOG_CHECKSUM_ALG_UNDEF)
    body_len_without_crc -= BINLOG_CHECKSUM_LEN;

  ret = packet_struct_->rotate_event.Parse(body_buf_, pos_, body_len_without_crc);

  packet_struct_->next_binlog_file_name = packet_struct_->rotate_event.next_binlog_filename_;
  packet_struct_->next_binlog_pos = packet_struct_->rotate_event.next_binlog_pos_;

  LOG(INFO) << " get rotate event, nextBinlogFileName:" << packet_struct_->next_binlog_pos << ", nextBinlogPos:"
            << packet_struct_->next_binlog_pos;
  return ret;
}

int NormalPacket::ParseFormatEvent() {
  int ret = 0;

  packet_struct_->table_id2tablemap_event.clear();
  ret = packet_struct_->format_event.Parse(body_buf_, pos_, body_len_, packet_struct_->checksum_enable);
  return ret;
}

int NormalPacket::ParseTablemapEvent() {
  int ret = 0;

  uint32_t body_len_without_crc = body_len_;
  if (packet_struct_->checksum_enable == true && packet_struct_->format_event.alg_ != BINLOG_CHECKSUM_ALG_OFF
      && packet_struct_->format_event.alg_ != BINLOG_CHECKSUM_ALG_UNDEF)
    body_len_without_crc -= BINLOG_CHECKSUM_LEN;

  uint64_t table_id = TablemapEvent::GetTableid(body_buf_, pos_, &packet_struct_->format_event);
  TablemapEvent *tablemap_event = NULL;
  if (packet_struct_->table_id2tablemap_event.find(table_id) == packet_struct_->table_id2tablemap_event.end()) {
    tablemap_event = new TablemapEvent();
    if (tablemap_event == NULL) {
      LOG(ERROR) << "NormalPacket::ParseTablemapEvent new TablemapEvent error." << std::endl;
      return -1;
    }
    packet_struct_->table_id2tablemap_event[table_id] = tablemap_event;
  } else {
    tablemap_event = packet_struct_->table_id2tablemap_event[table_id];
  }
  ret = tablemap_event->Parse(body_buf_, pos_, body_len_without_crc, &packet_struct_->format_event);

  return ret;
}

int NormalPacket::ParseRowEvent() {
  int ret = 0;

  uint32_t body_len_without_crc = body_len_;
  if (packet_struct_->checksum_enable && packet_struct_->format_event.alg_ != BINLOG_CHECKSUM_ALG_OFF
      && packet_struct_->format_event.alg_ != BINLOG_CHECKSUM_ALG_UNDEF)
    body_len_without_crc -= BINLOG_CHECKSUM_LEN;

  uint64_t table_id =
      RowEvent::GetTableid(body_buf_, pos_, packet_struct_->event_comm_head.event_type_, &packet_struct_->format_event);
  if (packet_struct_->table_id2tablemap_event.find(table_id) == packet_struct_->table_id2tablemap_event.end()) {
    LOG(ERROR) << "NormalPacket::ParseRowEvent row_event no found the right tabmap_event. tableid" << table_id
               << std::endl;
    return -1;
  }
  TablemapEvent *tablemap_event = packet_struct_->table_id2tablemap_event[table_id];

  packet_struct_->row_event.Init(packet_struct_->event_comm_head.event_type_);
  ret = packet_struct_->row_event.Parse(body_buf_,
                                        pos_,
                                        body_len_without_crc,
                                        &packet_struct_->format_event,
                                        tablemap_event,
                                        packet_struct_);

  return ret;
}

/*
 * Packet
 * */
int Packet::Init(bool checksum_enable) {
  packet_struct_ = std::make_shared<PacketStruct>();
  if (packet_struct_ == nullptr)
    return -1;

  packet_struct_->checksum_enable = checksum_enable;

  type_2_packet.clear();
  ErrorPacket *error_packet = new ErrorPacket();
  if (error_packet == nullptr)
    return -1;

  NormalPacket *normal_packet = new NormalPacket(packet_struct_);
  if (normal_packet == nullptr)
    return -1;

  type_2_packet[0xff] = (PacketBase *) error_packet;
  type_2_packet[0x00] = (PacketBase *) normal_packet;

  memset(head_buf_, 0, sizeof(head_buf_));
  body_buf_len_ = 4096;
  body_buf_ = (char *) malloc(body_buf_len_);
  if (body_buf_ == NULL)
    return -1;

  body_len_ = 0;
}

int Packet::Read(int fd) {
  int ret;
  int pos = 0;

  while (pos != head_len_) {
    ret = read(fd, head_buf_ + pos, 4 - pos);
    if (ret == -1) {
      return -1;
    } else if (ret == 0) {
      return 0;
    } else {
      pos += ret;
    }
  }

  body_len_ = uint3korr(head_buf_);
  LOG(ERROR) << __FUNCTION__ << " read:" << body_len_ << " data " << std::endl;
  if (-1 == ReallocBuf(body_len_)) {
    return -1;
  }

  pos = 0;
  while (pos != (int) body_len_) {
    ret = read(fd, body_buf_ + pos, body_len_ - pos);
    if (ret == -1) {
      return -1;
    } else if (ret == 0) {
      return 0;
    } else {
      pos += ret;
    }
  }

  return head_len_ + body_len_;
}

int Packet::Parse() {
  uint8_t err_flag = (uint8_t) body_buf_[0];
  if (err_flag != 0x00 && err_flag != 0xff)
    return -1;

  uint8_t seq = (uint8_t) head_buf_[3];
  type_2_packet[err_flag]->Init(body_len_, seq, err_flag, body_buf_);
  type_2_packet[err_flag]->Parse();

  if (err_flag == 0xff)
    return -1;
  return 0;
}

int Packet::ReallocBuf(uint32_t new_size) {
  if (new_size <= body_buf_len_)
    return 0;

  body_buf_ = (char *) realloc(body_buf_, new_size);
  if (body_buf_ == nullptr) {
    LOG(ERROR) << "Package::ReallocBuf  realloc error." << std::endl;
    return -1;
  }

  return 0;
}
