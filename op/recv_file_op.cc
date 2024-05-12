#include "op/recv_file_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"
#include <algorithm>
#include <asio/error.hpp>
#include <asio/ip/tcp.hpp>
#include <cerrno>
#include <cstddef>
#include <fcntl.h>
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

namespace orange {

RecvFileOp::RecvFileOp(SessionContext* context) :
  BasicOp(context),
  fd_(0),
  pipe_fd_{0} {
  if (context->request().body.empty()) {
    write_message(Response::get_code_string(ret_code::upload_fail, "Could not create file."));
    return;
  }
  fs::path target_path = context_->get_target_path(context->request().body);
  // for anonymous user, disable overwrite
  if (context->is_anonymous() && fs::exists(target_path)) {
    spdlog::warn("Anonymous user Cannot overwrite exist file");
    write_message(Response::get_code_string(ret_code::upload_fail, "Could not create file."));
    return;
  }
  file_name_ = target_path.string();
  fd_ = ::open(file_name_.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd_ < 0) {
    spdlog::critical("Open file failed: {}, error: {}", file_name_, asio::error_code(errno, asio::error::get_system_category()).message());
    write_message(Response::get_code_string(ret_code::upload_fail, "Could not create file."));
    return;
  }
  if (::pipe(pipe_fd_) < 0) {
    spdlog::critical("Create pipe failed, error: {}", asio::error_code(errno, asio::error::get_system_category()).message());
    write_message(Response::get_code_string(ret_code::upload_fail, "Could not create file."));
    return;
  }
}

RecvFileOp::~RecvFileOp() {
  if (fd_ > 0) {
    ::close(fd_);
    spdlog::info("Close file: {}", file_name_);
  }
  if (pipe_fd_[0] > 0) {
    ::close(pipe_fd_[1]);
    ::close(pipe_fd_[0]);
  }
  if (context_->data_socket() && context_->data_socket()->is_open()) {
    context_->data_socket()->close();
    spdlog::warn("Data socket is not closed when destroy RecvFileOp");
  }
}

void RecvFileOp::do_operation() {
  if (fd_ <= 0) {
    return;
  }
  auto self = shared_from_base<RecvFileOp>();
  set_completion_token([this](){
    write_message(Response::get_code_string(ret_code::transfer_complete));
    context_->clear_data_socket();
  });
  fetch_passive_connection<RecvFileOp>([self](){
    self->write_message(Response::get_code_string(ret_code::data_conn), [self](){
      self->do_recv_file();
    });
  });
}

void RecvFileOp::do_recv_file() {
  auto self = shared_from_base<RecvFileOp>();
  context_->data_socket()->async_wait(asio::ip::tcp::socket::wait_read, [self](std::error_code ec){
    self->deal_event(ec);
  });
}

void RecvFileOp::deal_event(std::error_code ec) {
  if (ec) {
    spdlog::error("Error occur: {}", ec.message());
    return;
  }
  if (!context_->data_socket()->native_non_blocking()) {
    spdlog::warn("Data socket is not non-blocking, setting..");
    context_->data_socket()->native_non_blocking(true);
  }
  spdlog::info("Available reading bytes: {}", context_->data_socket()->available());
  if (context_->data_socket()->available() == 0) {
    spdlog::info("Remote peer closed");
    if (completion_token_) {
      completion_token_();
    }
    return;
  }
  const size_t len = 65536;
  while(auto splice_ret = ::splice(context_->data_socket()->native_handle(), nullptr, pipe_fd_[1], nullptr, len, SPLICE_F_NONBLOCK | SPLICE_F_MORE | SPLICE_F_MOVE)) {
    if (splice_ret < 0) {
      std::error_code ec = asio::error_code(errno, asio::error::get_system_category());
      if (ec == asio::error::interrupted) {
        continue;
      } else if (ec == asio::error::would_block || ec == asio::error::try_again) {
        spdlog::info("No more data can be read");
        break;
      } else {
        spdlog::error("Error when calling splice: {}", ec.message());
        return;
      }
    }
    if (splice_ret == 0) {
      break;
    }
    auto bytes_in_pipe = splice_ret;
    spdlog::info("Read from socket: {} bytes", splice_ret);
    while (bytes_in_pipe > 0) {
      auto pipe_ret = ::splice(pipe_fd_[0], nullptr, fd_, nullptr, bytes_in_pipe, SPLICE_F_NONBLOCK | SPLICE_F_MORE | SPLICE_F_MOVE);
      spdlog::info("Read from pipe: {} bytes", pipe_ret);
      bytes_in_pipe -= pipe_ret;
    }
  }
  do_recv_file();
}

}
