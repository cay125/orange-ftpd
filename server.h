#pragma once

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <cstdint>

namespace orange {

class Server {
 public:
  Server(asio::io_context* io_context, uint16_t port);
  void start();

 private:
  void do_accept();

  asio::io_context* io_context_;
  uint16_t port_;
  asio::ip::tcp::acceptor acceptor_;
  bool stoped_ = false;
};

}  // namespace orange