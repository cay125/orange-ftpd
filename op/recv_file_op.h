#pragma once

#include "op/basic_op.h"
#include <string>
#include <system_error>

namespace orange {

class RecvFileOp : public BasicOp {
 public:
  RecvFileOp(SessionContext* context);
  ~RecvFileOp() override;

  void do_operation() override;

 private:
  void do_recv_file();
  void deal_event(std::error_code ec);
  
  int fd_;
  std::string file_name_;
  int pipe_fd_[2];
};

}