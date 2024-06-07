#pragma once

#include "session/context.h"
#include "utils/regist_factory.h"
#include <functional>
#include <memory>

namespace orange {

class BasicOp : public std::enable_shared_from_this<BasicOp> {
 public:
  BasicOp(SessionContext* context);
  virtual ~BasicOp();
  virtual void do_operation() = 0;
  virtual std::string name();
  bool is_stoped();
  void set_completion_token(std::function<void(void)> fun);
  void write_message(SharedConstBuffer buffer, std::function<void(void)> toekn = nullptr);

  template<typename T>
  std::shared_ptr<T> shared_from_base() {
    return std::dynamic_pointer_cast<T>(shared_from_this());
  }

  template<typename T>
  void fetch_passive_connection(std::function<void(void)> fcn) {
    if (!check_data_port()) {
      return;
    }
    auto self = shared_from_base<T>();
    context_->acceptor()->async_accept([self, f = std::move(fcn)](const std::error_code& ec, asio::ip::tcp::socket socket){
      if (ec == asio::error::operation_aborted) {
        spdlog::info("acceptor closed, port: {}", self->context_->acceptor()->local_endpoint().port());
        return;
      }
      self->context_->acceptor()->close();
      if (!ec) {
        socket.non_blocking(true);
        socket.native_non_blocking(true);
        self->context_->set_data_socket(std::move(socket));
        if (f) {
          f();
        }
      }
    });
  }

 protected:
  std::function<void(void)> completion_token_;
  SessionContext* context_;

 private:
  bool check_data_port();
};

}

#define REGISTER_FTPD_OP(derive_type, name) \
  REGISTER_PLUGIN(orange::BasicOp, derive_type, name, orange::SessionContext*);
