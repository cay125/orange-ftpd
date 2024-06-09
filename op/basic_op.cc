#include "op/basic_op.h"
#include "config/config.h"
#include "session/code.h"
#include "session/session.h"
#include <algorithm>
#include <chrono>
#include <spdlog/spdlog.h>

namespace orange {

BasicOp::BasicOp(SessionContext* context) : context_(context) {
  ++(*context_->mutable_active_op_num());
}

BasicOp::~BasicOp() {
  --(*context_->mutable_active_op_num());
  if (!context_->exists_pending_operator()) {
    if (!is_stoped()) {
      spdlog::info("All op destroyed, eestart idle_timer, remote: {}", context_->session()->remote_peer());
      context_->session()->update_idle_timer(std::chrono::seconds(Config::instance()->get_max_idle_timeout()));
    } else {
      context_->session()->cancel_idle_timer();
    }
  }
}

std::string BasicOp::name() {
  return "basic_op";
}

bool BasicOp::is_stoped() {
  return context_->session()->is_stoped();
}

void BasicOp::set_completion_token(std::function<void ()> fun) {
  completion_token_ = fun;
}

bool BasicOp::check_data_port() {
  if (context_->acceptor()->is_open()) {
    return true;
  }
  spdlog::info("Data connection port is not opened");
  context_->write_message(Response::get_code_string(ret_code::invalid_seq));
  return false;
}

void BasicOp::write_message(SharedConstBuffer buffer, std::function<void(void)> token) {
  context_->write_message(buffer, token);
}

}