#pragma once

#include "session/context.h"
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>
#include <asio/streambuf.hpp>
#include <memory>

namespace orange {

class Session : public std::enable_shared_from_this<Session> {
 public:
  Session(asio::io_context* io_context, asio::ip::tcp::socket socket);
  ~Session();
  void start();
  void close();

 private:
  void do_read();
  void handle_read(size_t n);
  void start_idle_timer();
  void dispatch_task();

  asio::io_context* io_context_;
  asio::ip::tcp::acceptor acceptor_;
  asio::streambuf buf;
  const char* delimit = "\r\n";
  asio::steady_timer check_idle_timer_;
  std::unique_ptr<SessionContext> context_;
};

}  // namespace orange
