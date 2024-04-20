#include "op/send_file_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"
#include <asio/detached.hpp>
#include <asio/error.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/write.hpp>
#include <cstddef>
#include <fcntl.h>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <sys/sendfile.h>
#include <system_error>
#include <unistd.h>

namespace fs = std::filesystem;

namespace orange {

SendFileOp::SendFileOp(SessionContext* context) :
  BasicOp(context),
  fd_(0),
  file_size_(0),
  file_offset_(0) {
  if (context->request().body.empty()) {
    asio::async_write(*context->control_socket(), Response::get_code_string(ret_code::noperm_filefatal, "File not found"), asio::detached);
    return;
  }
  fs::path target_path = fs::path(context->request().body);
  if (!target_path.is_absolute()) {
    target_path = context_->current_path() / target_path;
  }
  if (!fs::exists(target_path)) {
    asio::async_write(*context->control_socket(), Response::get_code_string(ret_code::noperm_filefatal, "File not found"), asio::detached);
    return;
  }
  file_name_ = target_path.string();
  file_size_ = fs::file_size(file_name_);
  fd_ = ::open(file_name_.c_str(), O_RDONLY);
  if (fd_ < 0) {
    spdlog::critical("Open file failed: {}, error: {}", file_name_, asio::error_code(errno, asio::error::get_system_category()).message());
    asio::async_write(*context->control_socket(), Response::get_code_string(ret_code::noperm_filefatal, "Failed to open file."), asio::detached);
    return;
  }
}

SendFileOp::~SendFileOp() {
  if (fd_ > 0) {
    ::close(fd_);
    spdlog::info("Close file: {}", file_name_);
  }
  if (context_->data_socket() && context_->data_socket()->is_open()) {
    context_->data_socket()->close();
    spdlog::warn("Data socket is not closed when destroy SendFileOp");
  }
}

void SendFileOp::do_operation() {
  if (fd_ <= 0) {
    return;
  }
  if (!check_data_port()) {
    return;
  }
  auto self = shared_from_base<SendFileOp>();
  set_completion_token([this](){
    asio::async_write(*context_->control_socket(), Response::get_code_string(ret_code::transfer_complete), asio::detached);
    context_->clear_data_socket();
  });
  context_->acceptor()->async_accept([self](const std::error_code& ec, asio::ip::tcp::socket socket){
    if (ec == asio::error::operation_aborted) {
      spdlog::info("acceptor closed, port: {}", self->context_->acceptor()->local_endpoint().port());
      return;
    }
    self->context_->acceptor()->close();
    if (!ec) {
      socket.non_blocking(true);
      socket.native_non_blocking(true);
      self->context_->set_data_socket(std::move(socket));
      asio::async_write(*self->context_->control_socket(), Response::get_code_string(ret_code::data_conn), [self](std::error_code ec, size_t n){
        self->do_send_file();
      });
    }
  });
}

void SendFileOp::do_send_file() {
  auto self = shared_from_base<SendFileOp>();
  context_->data_socket()->async_wait(asio::ip::tcp::socket::wait_write, [self](std::error_code ec){
    self->deal_event(ec);
  });
}

void SendFileOp::deal_event(std::error_code ec) {
  if (ec) {
    spdlog::error("Error occur: {}", ec.message());
    return;
  }
  if (!context_->data_socket()->native_non_blocking()) {
    spdlog::warn("Data socket is not non-blocking, setting..");
    context_->data_socket()->native_non_blocking(true);
  }
  auto ret = ::sendfile(context_->data_socket()->native_handle(), fd_, &file_offset_, file_size_ - file_offset_);
  if (ret < 0) {
    std::error_code ec = asio::error_code(errno, asio::error::get_system_category());
    if (ec == asio::error::would_block || ec == asio::error::try_again) {
      spdlog::info("No more data can be written");
      do_send_file();
    } else {
      spdlog::info("Error occur: {}", ec.message());
    }
    return;
  }
  spdlog::info("Send file: {} {} bytes, total_send: {}, left: {}", file_name_, ret, file_offset_, file_size_ - file_offset_);
  if ((file_size_ - file_offset_) > 0) {
    do_send_file();
  } else {
    if (completion_token_) {
      completion_token_();
    }
  }
}

}
