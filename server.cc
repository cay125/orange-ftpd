#include "server.h"
#include "config/config.h"
#include "session/session.h"
#include <algorithm>
#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <cstdint>
#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <system_error>
#include <thread>

namespace orange {

using asio::ip::tcp;

Server::Server(uint16_t port) :
  work_index_(0),
  port_(port),
  acceptor_(io_context_, tcp::endpoint(tcp::v4(), port_)) {
  if (!acceptor_.is_open()) {
    throw std::logic_error("Bind port failed");
  }
  spdlog::info("Hardware_concurrency is: {}", std::thread::hardware_concurrency());
  for (uint i = 0; i < std::thread::hardware_concurrency(); i++) {
    works_.emplace_back(std::make_unique<Worker>());
  }
  // start worker
  std::for_each(works_.begin(), works_.end(), [](std::unique_ptr<Worker>& worker){
    worker->thread = std::thread([&worker](){
      auto work_guard = asio::make_work_guard(worker->io_context);
      worker->io_context.run();
    });
  });
}

void Server::start() {
  do_accept();
  io_context_.run();
  spdlog::warn("Server terminated");
}

void Server::do_accept() {
  asio::io_context& io_context = works_[(work_index_++) % works_.size()]->io_context;
  acceptor_.async_accept(io_context, [this, &io_context](const std::error_code& ec, asio::ip::tcp::socket socket){
    if (!ec) {
      Config::instance()->inc_client_num();
      auto session = std::make_shared<Session>(&io_context, std::move(socket));
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