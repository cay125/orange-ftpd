#include "op/ack_op.h"
#include "session/code.h"

namespace orange {

AckOp::AckOp(SessionContext* context) :
  BasicOp(context) {

}

AckOp::~AckOp() {

}

void AckOp::do_operation() {
  // do nothong, but send ok
  write_message(Response::get_code_string(ret_code::ok, "NOOP ok"));
}

std::string AckOp::name() {
  return "ack_op";
}

}