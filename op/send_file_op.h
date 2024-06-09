#pragma once

#include "op/basic_op.h"
#include <asio/high_resolution_timer.hpp>
#include <cstddef>
#include <string>
#include <system_error>

namespace orange {

class SendFileOp : public BasicOp {
 public:
  SendFileOp(SessionContext* context);
  ~SendFileOp() override;

  void do_operation() override;

 private:
  void do_send_file();
  void deal_event(std::error_code ec, size_t slice_size);

  int fd_;
  std::string file_name_;
  size_t file_size_;
  off_t file_offset_;
  asio::high_resolution_timer limiter_timer_;
  static constexpr int time_splice = 10;
};

}