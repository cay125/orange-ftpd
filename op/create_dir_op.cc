#include "config/config.h"
#include "op/create_dir_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/ftp_request.h"
#include <asio/detached.hpp>
#include <asio/write.hpp>
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
    asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::noperm_filefatal, "Permission denied."), asio::detached);
    return;
  }
  const FtpRequest& request = context_->request();
  if (request.body.empty()) {
    asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::noperm_filefatal, "Create directory operation failed."), asio::detached);
    return;
  }
  fs::path target_path = fs::path(request.body);
  if (!target_path.is_absolute()) {
    target_path = context_->current_path() / target_path;
  }
  if (fs::exists(target_path)) {
    asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::noperm_filefatal, "Create directory operation failed."), asio::detached);
    return;
  }
  fs::create_directory(target_path);
  if (fs::exists(target_path) && fs::is_directory(target_path)) {
    asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::pwd_or_mkdir_ok,"\"" + fs::canonical(target_path).string()  + std::string("\" created.")), asio::detached);
  } else {
    asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::noperm_filefatal, "Create directory operation failed."), asio::detached);
  }
}

}
