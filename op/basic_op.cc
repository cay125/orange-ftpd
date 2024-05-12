#include "op/basic_op.h"
#include "session/code.h"

namespace orange {

BasicOp::BasicOp(SessionContext* context) : context_(context) {

}

BasicOp::~BasicOp() {

}

std::string BasicOp::name() {
  return "basic_op";
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