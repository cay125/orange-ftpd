#pragma once

#include "session/code.h"
#include "session/ftp_request.h"
#include <algorithm>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <filesystem>
#include <type_traits>
#include <utility>

namespace fs = std::filesystem;

namespace orange {

enum class transport_type {
  ascii,
  binary,
};

#define field_guard(name, type)                                    \
 public:                                                           \
  decltype(auto) name() const {                                    \
    if constexpr (!std::is_scalar_v<type>) {                       \
      const auto& ref = std::as_const(name##_);                    \
      return (ref);                                                \
    } else {                                                       \
     return name##_;                                               \
    }                                                              \
  }                                                                \
  type* mutable_##name() {                                         \
    return &name##_;                                               \
  }                                                                \
  void set_##name(type value) {                                    \
    name##_ = value;                                               \
  }                                                                \
 private:                                                          \
  type name##_;

using Message = std::pair<SharedConstBuffer, std::function<void(void)>>;

class SessionContext {
 public:
  SessionContext();
  SessionContext(const SessionContext&) = delete;
  SessionContext& operator=(const SessionContext& context) = delete;

  asio::ip::tcp::socket* data_socket() {
    return data_socket_.get();
  }

  asio::ip::tcp::socket* control_socket() {
    return control_socket_.get();
  }

  asio::ip::tcp::acceptor* acceptor() {
    return acceptor_.get();
  }

  fs::path get_target_path(const std::string& path);

  void clear_data_socket() {
    data_socket_->close();
    data_socket_.reset();
  }

  void set_data_socket(asio::ip::tcp::socket socket) {
    data_socket_ = std::make_unique<asio::ip::tcp::socket>(std::move(socket));
  }

  void set_control_socket(asio::ip::tcp::socket socket) {
    control_socket_ = std::make_unique<asio::ip::tcp::socket>(std::move(socket));
  }

  void set_acceptor(asio::io_context* io_context) {
    acceptor_ = std::make_unique<asio::ip::tcp::acceptor>(*io_context);
  }

  void write_message(SharedConstBuffer buffer, std::function<void(void)> toekn = nullptr);

  field_guard(current_path, fs::path);
  field_guard(verified, bool);
  field_guard(command, std::string);
  field_guard(user, std::string);
  field_guard(password, std::string);
  field_guard(enable_write, bool);
  field_guard(request, FtpRequest);
  field_guard(is_anonymous, bool);
  field_guard(type, transport_type);
  field_guard(message_queue, std::queue<Message>);

 private:
  void do_write_message();

  std::unique_ptr<asio::ip::tcp::socket> data_socket_;
  std::unique_ptr<asio::ip::tcp::socket> control_socket_;
  std::unique_ptr<asio::ip::tcp::acceptor> acceptor_;
};

}