#pragma once

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

namespace orange {

class Server {
 public:
  Server(uint16_t port);
  void start();

 private:
  void do_accept();

  struct Worker {
    std::thread thread;
    asio::io_context io_context;
  };
  std::vector<std::unique_ptr<Worker>> works_;
  uint work_index_;
  asio::io_context io_context_;
  uint16_t port_;
  asio::ip::tcp::acceptor acceptor_;
  bool stoped_ = false;
};

}  // namespace orange