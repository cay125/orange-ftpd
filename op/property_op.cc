#include "op/property_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"
#include "session/ftp_request.h"
#include <algorithm>
#include <asio/detached.hpp>
#include <asio/write.hpp>
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
      asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::ok, "Switching to Binary mode."), asio::detached);
      return;
    }
    if (request.body == "A" ||
        request.body == "A N") {
      context_->set_type(transport_type::ascii);
      asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::ok, "Switching to ASCII mode."), asio::detached);
      return;
    }
    asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::syntax_error, "Unrecognised TYPE command."), asio::detached);
  } else if (request.command == "OPTS") {
    if (request.body == "UTF8 ON") {
      asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::ok, "Always in UTF8 mode."), asio::detached);
      return;
    }
    asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::bad_opts, "Option not understood."), asio::detached);
  }
}

std::string PropertyOp::name() {
  return "property_op";
}

}