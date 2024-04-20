#include "op/change_working_dir_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"
#include <asio/detached.hpp>
#include <asio/write.hpp>
#include <spdlog/spdlog.h>

namespace orange {

ChangeWorkingDirOp::ChangeWorkingDirOp(SessionContext* context) :
  BasicOp(context) {

}

ChangeWorkingDirOp::~ChangeWorkingDirOp() {

}

void ChangeWorkingDirOp::do_operation() {
  if (context_->request().body.empty()) {
    asio::async_write(*context_->control_socket(), Response::get_cwd_response(&context_->current_path()), asio::detached);
  } else {
    fs::path target_path = fs::path(context_->request().body);
    if (!target_path.is_absolute()) {
      target_path = context_->current_path() / target_path;
    }
    if (fs::exists(target_path) && fs::is_directory(target_path)) {
      context_->set_current_path(fs::canonical(target_path));
      asio::async_write(*context_->control_socket(), Response::get_cwd_response(&context_->current_path()), asio::detached);
    } else {
      asio::async_write(*context_->control_socket(), Response::get_cwd_response(nullptr), asio::detached);
    }
  }
}

std::string ChangeWorkingDirOp::name() {
  return "change_working_dir_op";
}

}