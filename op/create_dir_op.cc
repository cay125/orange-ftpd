#include "config/config.h"
#include "op/create_dir_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/ftp_request.h"
#include <filesystem>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

namespace orange {

CreateDirOp::CreateDirOp(SessionContext* context) :
  BasicOp(context) {
}

CreateDirOp::~CreateDirOp() {
}

std::string CreateDirOp::name() {
  return "create_dir_op";
}

void CreateDirOp::do_operation() {
  if (!Config::instance()->get_write_enable()) {
    write_message(Response::get_code_string(ret_code::noperm_filefatal, "Permission denied."));
    return;
  }
  const FtpRequest& request = context_->request();
  if (request.body.empty()) {
    write_message(Response::get_code_string(ret_code::noperm_filefatal, "Create directory operation failed."));
    return;
  }
  fs::path target_path = context_->get_target_path(request.body);
  if (fs::exists(target_path)) {
    write_message(Response::get_code_string(ret_code::noperm_filefatal, "Create directory operation failed."));
    return;
  }
  fs::create_directory(target_path);
  if (fs::exists(target_path) && fs::is_directory(target_path)) {
    write_message(Response::get_code_string(ret_code::pwd_or_mkdir_ok,"\"" + fs::canonical(target_path).string()  + std::string("\" created.")));
  } else {
    write_message(Response::get_code_string(ret_code::noperm_filefatal, "Create directory operation failed."));
  }
}

}
