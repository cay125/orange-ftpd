#pragma once

#include "session/context.h"
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>
#include <asio/streambuf.hpp>
#include <memory>

namespace orange {

class BasicOp;

class Session : public std::enable_shared_from_this<Session> {
  friend class BasicOp;
 public:
  Session(asio::io_context* io_context, asio::ip::tcp::socket socket);
  ~Session();
  void start();
  void close();
  void update_idle_timer(std::chrono::seconds seconds);
  bool is_stoped();

 private:
  void do_read();
  void handle_read(size_t n);
  void start_idle_timer();
  bool dispatch_task();

  asio::io_context* io_context_;
  asio::streambuf buf;
  const char* delimit = "\r\n";
  asio::steady_timer check_idle_timer_;
  std::unique_ptr<SessionContext> context_;
  bool stoped_;
};

}  // namespace orange
