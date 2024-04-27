#include "server.h"
#include "config/config.h"
#include "session/session.h"
#include <asio/ip/tcp.hpp>
#include <cstdint>
#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <system_error>

namespace orange {

using asio::ip::tcp;

Server::Server(asio::io_context* io_context, uint16_t port) : 
  io_context_(io_context),
  port_(port),
  acceptor_(*io_context, tcp::endpoint(tcp::v4(), port_)) {
  if (!acceptor_.is_open()) {
    throw std::logic_error("Bind port failed");
  }
}

void Server::start() {
  do_accept();
}

void Server::do_accept() {
  acceptor_.async_accept([this](const std::error_code& ec, asio::ip::tcp::socket socket){
    if (!ec) {
      Config::instance()->inc_client_num();
      auto session = std::make_shared<Session>(io_context_, std::move(socket));
      session->start();
    } else {
      spdlog::error("Error happen: {}", ec.message());
    }
    if (stoped_) {
      return;
    }
    do_accept();
  });
}

}  // namespace orange