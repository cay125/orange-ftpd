#include "op/property_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"
#include "session/ftp_request.h"
#include <algorithm>
#include <cctype>
#include <spdlog/spdlog.h>

namespace orange {

PropertyOp::PropertyOp(SessionContext* context) :
  BasicOp(context) {

}

PropertyOp::~PropertyOp() {

}

void PropertyOp::do_operation() {
  std::transform(context_->mutable_request()->body.begin(),
                 context_->mutable_request()->body.end(),
                 context_->mutable_request()->body.begin(),
                 [](char c){return std::toupper(c);});
  const FtpRequest& request = context_->request();
  if (request.command == "TYPE") {
    if (request.body == "I" ||
        request.body == "L8" ||
        request.body == "L 8") {
      context_->set_type(transport_type::binary);
      write_message(Response::get_code_string(ret_code::ok, "Switching to Binary mode."));
      return;
    }
    if (request.body == "A" ||
        request.body == "A N") {
      context_->set_type(transport_type::ascii);
      write_message(Response::get_code_string(ret_code::ok, "Switching to ASCII mode."));
      return;
    }
    write_message(Response::get_code_string(ret_code::syntax_error, "Unrecognised TYPE command."));
  } else if (request.command == "OPTS") {
    if (request.body == "UTF8 ON") {
      write_message(Response::get_code_string(ret_code::ok, "Always in UTF8 mode."));
      return;
    }
    write_message(Response::get_code_string(ret_code::bad_opts, "Option not understood."));
  }
}

std::string PropertyOp::name() {
  return "property_op";
}

}