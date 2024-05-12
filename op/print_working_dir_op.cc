#include "op/print_working_dir_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"

namespace orange {

PrintWorkingDirOp::PrintWorkingDirOp(SessionContext* context) :
  BasicOp(context) {

}

PrintWorkingDirOp::~PrintWorkingDirOp() {

}

void PrintWorkingDirOp::do_operation() {
  write_message(Response::get_pwd_response(&context_->current_path()));
}

std::string PrintWorkingDirOp::name() {
  return "print_working_dir_op";
}

}