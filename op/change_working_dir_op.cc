#include "op/change_working_dir_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"
#include <spdlog/spdlog.h>

namespace orange {

ChangeWorkingDirOp::ChangeWorkingDirOp(SessionContext* context) :
  BasicOp(context) {

}

ChangeWorkingDirOp::~ChangeWorkingDirOp() {

}

void ChangeWorkingDirOp::do_operation() {
  if (context_->request().body.empty()) {
    write_message(Response::get_cwd_response(&context_->current_path()));
  } else {
    fs::path target_path = context_->get_target_path(context_->request().body);
    if (fs::exists(target_path) && fs::is_directory(target_path)) {
      context_->set_current_path(fs::canonical(target_path));
      write_message(Response::get_cwd_response(&context_->current_path()));
    } else {
      write_message(Response::get_cwd_response(nullptr));
    }
  }
}

std::string ChangeWorkingDirOp::name() {
  return "change_working_dir_op";
}

}