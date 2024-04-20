#include "op/basic_op.h"
#include "session/code.h"
#include <asio/detached.hpp>
#include <asio/write.hpp>

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
  asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::invalid_seq), asio::detached);
  return false;
}

}