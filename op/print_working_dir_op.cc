#include "op/print_working_dir_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"
#include <asio/detached.hpp>
#include <asio/write.hpp>

namespace orange {

PrintWorkingDirOp::PrintWorkingDirOp(SessionContext* context) :
  BasicOp(context) {

}

PrintWorkingDirOp::~PrintWorkingDirOp() {

}

void PrintWorkingDirOp::do_operation() {
  asio::async_write(*context_->control_socket(), Response::get_pwd_response(&context_->current_path()), asio::detached); 
}

std::string PrintWorkingDirOp::name() {
  return "print_working_dir_op";
}

}