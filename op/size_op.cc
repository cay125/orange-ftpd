#include "op/size_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"
#include <filesystem>
#include <string>

namespace orange {

SizeOp::SizeOp(SessionContext* context) :
  BasicOp(context) {

}

SizeOp::~SizeOp() {

}

void SizeOp::do_operation() {
  auto path = context_->get_target_path(context_->request().body);
  if (!fs::exists(path) || !fs::is_regular_file(path)) {
    write_message(Response::get_code_string(ret_code::noperm_filefatal, "Could not get file size."));
  } else {
    write_message(Response::get_code_string(ret_code::size_or_mdtm_ok, std::to_string(fs::file_size(path))));
  }
}

std::string SizeOp::name() {
  return "size_op";
}

}
