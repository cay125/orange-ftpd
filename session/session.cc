#include "asio/buffer.hpp"
#include "asio/read_until.hpp"
#include "config/config.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"
#include "session/ftp_request.h"
#include "session/session.h"
#include "asio/write.hpp"
#include "utils/regist_factory.h"
#include <algorithm>
#include <asio/buffers_iterator.hpp>
#include <asio/detached.hpp>
#include <asio/error.hpp>
#include <asio/error_code.hpp>
#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <vector>

namespace orange {

static std::array<std::string, 12> month_mapping = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sept", "Oct", "Nov", "Dec"
};

Session::Session(asio::io_context* io_context, asio::ip::tcp::socket socket) :
  io_context_(io_context),
  acceptor_(*io_context),
  check_idle_timer_(*io_context),
  context_(std::make_unique<SessionContext>()) {
  context_->set_control_socket(std::move(socket));
  context_->set_acceptor(io_context);
}

void Session::start() {
  auto self = shared_from_this();

  const std::string default_banner = "Welcome to ftpd.";
  std::string welcome_banner = Config::instance()->get_ftpd_banner().empty() ?
      default_banner : Config::instance()->get_ftpd_banner();
  asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::welcome, welcome_banner),
    [this, self](const std::error_code& ec, size_t n){
      if (!ec) {
        do_read();
      } else {
        spdlog::error("Write error: {}", ec.message());
      }
    }
  );
  check_idle_timer_.expires_after(std::chrono::seconds(Config::instance()->get_max_idle_timeout()));
  // start_idle_timer();
}

void Session::start_idle_timer() {
  check_idle_timer_.async_wait([this](const std::error_code& ec){
    if (ec == asio::error::operation_aborted) {
      if (check_idle_timer_.expiry() > asio::steady_timer::clock_type::now()) {
        start_idle_timer();
      }
    } else {
      if (ec) {
        spdlog::error("Timer error: {}", ec.message());
      } else {
        spdlog::info("Session closed by max_idle_timeout");
        close();
      }
    }
  });
}

void Session::do_read() {
  auto self = shared_from_this();
  asio::async_read_until(*context_->control_socket(), buf, delimit,
    [this, self](const std::error_code& ec, size_t n){
      if (ec) {
        if (ec == asio::error::eof) {
          spdlog::info("Connection close by remote, ip: {}, port: {}",
            context_->control_socket()->local_endpoint().address().to_string(), context_->control_socket()->local_endpoint().port());
        } else if (ec == asio::error::operation_aborted) {
          
        } else {
          spdlog::error("Read error: {}", ec.message());
        }
        return;
      }
      check_idle_timer_.expires_after(std::chrono::seconds(Config::instance()->get_max_idle_timeout()));
      handle_read(n);
      do_read();
    }
  );
}

void Session::close() {
  std::error_code ignored_err;
  context_->control_socket()->close(ignored_err);
  check_idle_timer_.cancel();
  if (acceptor_.is_open()) {
    acceptor_.close();
  }
}

static bool build_request(const std::vector<std::string> input, FtpRequest* request) {
  if (input.empty()) {
    return false;
  }
  request->command = input[0];
  std::transform(request->command.begin(), request->command.end(),
                 request->command.begin(),
                 [](char c){return std::toupper(c);});
  if (input.size() > 1) {
    std::stringstream ss;
    ss << input[1];
    for (size_t i = 2; i < input.size(); i++) {
      ss << " " << input[i];
    }
    request->body = ss.str();
  }
  return true;
}

void Session::handle_read(size_t n) {
  std::stringstream is(std::string(
    asio::buffers_begin(buf.data()),
    asio::buffers_begin(buf.data()) + n
  ));
  std::vector<std::string> ret;
  std::string line;
  while (is >> line) {
    ret.push_back(std::move(line));
  }
  spdlog::info("Receive data: {}", fmt::join(ret, " "));
  buf.consume(n);

  context_->mutable_request()->reset();
  if (!build_request(ret, context_->mutable_request())) {
    return;
  }

  dispatch_task();
}

void Session::dispatch_task() {
  const auto& request = context_->request();
  if (!context_->verified()) {
    auto op = PluginRegistry<BasicOp, SessionContext*>::create("AUTH", context_.get());
    op->do_operation();
    return;
  }
  if (request.command == "USER") {
    asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::not_allowed, "Can't change to another user."), asio::detached);
    return;
  }
  if (request.command == "PASS") {
    asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::logged_on, "Already logged in."), asio::detached);
    return;
  }
  auto op = PluginRegistry<BasicOp, SessionContext*>::create(request.command, context_.get());
  if (!op) {
    asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::syntax_error), asio::detached);
    return;
  }
  op->do_operation();
}

}  // namespace orange
