#include "session/context.h"
#include "config/config.h"
#include "session/session.h"
#include <asio/write.hpp>
#include <cstdlib>
#include <filesystem>
#include <spdlog/spdlog.h>

namespace orange {

SessionContext::SessionContext() :
  current_path_(fs::path(std::getenv("HOME"))),
  verified_(false),
  enable_write_(false),
  is_anonymous_(false),
  type_(transport_type::binary) {
}

SessionContext::~SessionContext() {
  if (acceptor_->is_open()) {
    spdlog::warn("Context destroy while acceptor opening");
    acceptor_->close();
  }
}

fs::path SessionContext::get_target_path(const std::string& path) {
  fs::path target_path = fs::path(path);
  if (!target_path.is_absolute()) {
    target_path = current_path() / target_path;
  }
  return target_path;
}

void SessionContext::write_message(SharedConstBuffer buffer, std::function<void(void)> token) {
  bool is_writing = !message_queue().empty();
  mutable_message_queue()->push(std::make_pair(buffer, token));
  if (!is_writing) {
    do_write_message();
  }
}

void SessionContext::do_write_message() {
  if (message_queue().empty()) {
    if (!exists_pending_operator()) {
      if (!session()->is_stoped()) {
        spdlog::info("All message delivered, restart idle_timer, remote: {}:{}", control_socket()->remote_endpoint().address().to_string(), control_socket()->remote_endpoint().port());
        session()->update_idle_timer(std::chrono::seconds(Config::instance()->get_max_idle_timeout()));
      } else {
        session()->cancel_idle_timer();
      }
    }
    return;
  }
  SharedConstBuffer buffer = message_queue().front().first;
  asio::async_write(*control_socket(), buffer, [this](std::error_code ec, size_t n){
    if (ec) {
      spdlog::error("Error happen when writing message: {}", ec.message());
      std::queue<Message> empty_queue;
      mutable_message_queue()->swap(empty_queue);
      return;
    }
    if (message_queue().front().second) {
      auto&& f = message_queue().front().second;
      f();
    }
    mutable_message_queue()->pop();
    do_write_message();
  });
}

bool SessionContext::exists_pending_operator() {
  if (active_op_num() == 0 && message_queue().empty()) {
    return false;
  }
  return true;
}

}