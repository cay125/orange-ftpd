#include "op/ack.h"
#include "session/code.h"
#include <asio/detached.hpp>
#include <asio/write.hpp>

namespace orange {

AckOp::AckOp(SessionContext* context) :
  BasicOp(context) {

}

AckOp::~AckOp() {

}

void AckOp::do_operation() {
  // do nothong, but send ok
  asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::ok, "NOOP ok"), asio::detached);
}

std::string AckOp::name() {
  return "ack_op";
}

}