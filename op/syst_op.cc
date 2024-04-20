#include "op/syst_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"
#include <asio/detached.hpp>
#include <asio/write.hpp>

namespace orange {

SystOp::SystOp(SessionContext* context) :
  BasicOp(context) {

}

SystOp::~SystOp() {

}

void SystOp::do_operation() {
  asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::syst_ok), asio::detached);
}

std::string SystOp::name() {
  return "syst_op";
}

}