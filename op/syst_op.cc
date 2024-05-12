#include "op/syst_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"

namespace orange {

SystOp::SystOp(SessionContext* context) :
  BasicOp(context) {

}

SystOp::~SystOp() {

}

void SystOp::do_operation() {
  write_message(Response::get_code_string(ret_code::syst_ok));
}

std::string SystOp::name() {
  return "syst_op";
}

}