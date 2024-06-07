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
#include <asio/error.hpp>
#include <asio/error_code.hpp>
#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <chrono>
#include <memory>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace orange {

static std::array<std::string, 12> month_mapping = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sept", "Oct", "Nov", "Dec"
};

Session::Session(asio::io_context* io_context, asio::ip::tcp::socket socket) :
  io_context_(io_context),
  check_idle_timer_(*io_context),
  context_(std::make_unique<SessionContext>()),
  stoped_(false) {
  context_->set_control_socket(std::move(socket));
  context_->set_acceptor(io_context);
  context_->set_session(this);
}

Session::~Session() {
  Config::instance()->dec_client_num();
  spdlog::info("Session destroyed");
}

void Session::update_idle_timer(std::chrono::seconds seconds) {
  check_idle_timer_.expires_after(seconds);
}

bool Session::is_stoped() {
  return stoped_;
}

void Session::start() {
  spdlog::info("Connection established, remote host: {}, port: {}", context_->control_socket()->remote_endpoint().address().to_string(), context_->control_socket()->remote_endpoint().port());
  auto self = shared_from_this();

  const std::string default_banner = "Welcome to ftpd.";
  std::string welcome_banner = Config::instance()->get_ftpd_banner().empty() ?
      default_banner : Config::instance()->get_ftpd_banner();
  asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::welcome, welcome_banner),
    [this, self](const std::error_code& ec, size_t n){
      if (!ec) {
        update_idle_timer(std::chrono::seconds(Config::instance()->get_max_idle_timeout()));
        do_read();
        start_idle_timer();
      } else {
        spdlog::error("Write error: {}", ec.message());
      }
    }
  );
}

void Session::start_idle_timer() {
  check_idle_timer_.async_wait([this](const std::error_code& ec){
    if (is_stoped()) {
      return;
    }
    if (check_idle_timer_.expiry() > asio::steady_timer::clock_type::now()) {
      start_idle_timer();
      return;
    }
    spdlog::info("Session closed by max_idle_timeout");
    close();
  });
}

void Session::do_read() {
  auto self = shared_from_this();
  asio::async_read_until(*context_->control_socket(), buf, delimit,
    [this, self](const std::error_code& ec, size_t n){
      if (ec) {
        if (ec == asio::error::eof) {
          spdlog::info("Connection close by remote");
          close();
          if (context_->active_op_num()) {
            spdlog::warn("Active operator exsits, num: {}", context_->active_op_num());
            check_idle_timer_.expires_at(std::chrono::steady_clock::time_point::max());
            check_idle_timer_.async_wait([self](std::error_code ec){
              spdlog::info("All operator finished, session is about to be destroyed");
            });
          }
        } else if (ec == asio::error::operation_aborted) {
          
        } else {
          spdlog::error("Read error: {}", ec.message());
        }
        return;
      }
      // cancel timer
      check_idle_timer_.expires_at(std::chrono::steady_clock::time_point::max());
      handle_read(n);
      do_read();
    }
  );
}

void Session::close() {
  stoped_ = true;
  std::error_code ignored_err;
  context_->control_socket()->close(ignored_err);
  check_idle_timer_.cancel();
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
  spdlog::info("Receive data: {} from user: {}, remote: {}", fmt::join(ret, " "), context_->user(), context_->control_socket()->remote_endpoint().address().to_string());
  buf.consume(n);

  context_->mutable_request()->reset();
  if (!build_request(ret, context_->mutable_request()) || !dispatch_task()) {
    update_idle_timer(std::chrono::seconds(Config::instance()->get_max_idle_timeout()));
  }
}

bool Session::dispatch_task() {
  const auto& request = context_->request();
  if (!context_->verified()) {
    auto op = PluginRegistry<BasicOp, SessionContext*>::create("AUTH", context_.get());
    op->do_operation();
    return true;
  }
  if (request.command == "USER") {
    context_->write_message(Response::get_code_string(ret_code::not_allowed, "Can't change to another user."));
    return false;
  }
  if (request.command == "PASS") {
    context_->write_message(Response::get_code_string(ret_code::logged_on, "Already logged in."));
    return false;
  }
  auto op = PluginRegistry<BasicOp, SessionContext*>::create(request.command, context_.get());
  if (!op) {
    context_->write_message(Response::get_code_string(ret_code::syntax_error));
    return false;
  }
  op->do_operation();
  return true;
}

}  // namespace orange
