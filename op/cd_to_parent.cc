#include "op/cd_to_parent.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"

namespace orange {

CdToParentOp::CdToParentOp(SessionContext* context) :
  BasicOp(context) {

}

CdToParentOp::~CdToParentOp() {

}

void CdToParentOp::do_operation() {
  fs::path target_path = context_->get_target_path("..");
  if (fs::exists(target_path) && fs::is_directory(target_path)) {
    context_->set_current_path(fs::canonical(target_path));
    write_message(Response::get_cwd_response(&context_->current_path()));
  } else {
    write_message(Response::get_cwd_response(nullptr));
  }
}

std::string CdToParentOp::name() {
  return "cd_to_parent_op";
}

}