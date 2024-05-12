#include "op/passive_mode_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"
#include <spdlog/spdlog.h>

namespace orange {

PassiveModeOp::PassiveModeOp(SessionContext* context) :
  BasicOp(context) {

}

PassiveModeOp::~PassiveModeOp() {

}

void PassiveModeOp::do_operation() {
  auto& acceptor = *context_->acceptor();
  if (acceptor.is_open()) {
    acceptor.close();
  }
  asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), 0);
  acceptor.open(endpoint.protocol());
  acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
  acceptor.bind(endpoint);
  acceptor.listen();
  spdlog::info("Data connection port listened on: {}", acceptor.local_endpoint().port());
  auto response = Response::get_pasv_response(
      context_->control_socket()->local_endpoint().address().to_string(),
      acceptor.local_endpoint().port());
  write_message(response);
}

std::string PassiveModeOp::name() {
  return "passive_mode_op";
}

}