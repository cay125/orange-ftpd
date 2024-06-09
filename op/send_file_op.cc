#include "op/send_file_op.h"
#include "config/config.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"
#include "session/session.h"
#include <algorithm>
#include <asio/error.hpp>
#include <asio/ip/tcp.hpp>
#include <chrono>
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
  file_offset_(0),
  limiter_timer_(*context->session()->get_io_context()) {
  if (context->request().body.empty()) {
    write_message(Response::get_code_string(ret_code::noperm_filefatal, "File not found"));
    return;
  }
  fs::path target_path = context_->get_target_path(context->request().body);
  if (!fs::exists(target_path)) {
    write_message(Response::get_code_string(ret_code::noperm_filefatal, "File not found"));
    return;
  }
  file_name_ = target_path.string();
  file_size_ = fs::file_size(file_name_);
  fd_ = ::open(file_name_.c_str(), O_RDONLY);
  if (fd_ < 0) {
    spdlog::critical("Open file failed: {}, error: {}", file_name_, asio::error_code(errno, asio::error::get_system_category()).message());
    write_message(Response::get_code_string(ret_code::noperm_filefatal, "Failed to open file."));
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
  auto self = shared_from_base<SendFileOp>();
  set_completion_token([this](){
    write_message(Response::get_code_string(ret_code::transfer_complete));
    context_->clear_data_socket();
  });
  fetch_passive_connection<SendFileOp>([self](){
    std::error_code ec;
    self->context_->data_socket()->set_option(asio::ip::tcp::no_delay(true));
    if (ec) {
      spdlog::error("Error when setting no_delay: {}", ec.message());
      self->write_message(Response::get_code_string(ret_code::bad_send_net, "Failure writing network stream."));
      self->context_->clear_data_socket();
      return;
    }
    self->write_message(Response::get_code_string(ret_code::data_conn), [self](){
      self->do_send_file();
    });
  });
}

void SendFileOp::do_send_file() {
  auto self = shared_from_base<SendFileOp>();
  context_->data_socket()->async_wait(asio::ip::tcp::socket::wait_write, [self](std::error_code ec){
    self->deal_event(ec, Config::instance()->get_download_speed_limit_in_kb_per_s() * 1024 / self->time_splice);
  });
}

void SendFileOp::deal_event(std::error_code ec, size_t slice_size) {
  if (ec) {
    spdlog::error("Error occur: {}", ec.message());
    return;
  }
  if (!context_->data_socket()->native_non_blocking()) {
    spdlog::warn("Data socket is not non-blocking, setting..");
    context_->data_socket()->native_non_blocking(true);
  }
  auto ret = ::sendfile(context_->data_socket()->native_handle(), fd_, &file_offset_, slice_size == 0 ? (file_size_ - file_offset_) : std::min(file_size_ - file_offset_, slice_size));
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
    if (slice_size == 0) {
      do_send_file();
    } else {
      auto self(shared_from_base<SendFileOp>());
      limiter_timer_.expires_after(std::chrono::milliseconds(1000 / time_splice));
      limiter_timer_.async_wait([self](std::error_code ec){
        self->deal_event(ec, Config::instance()->get_download_speed_limit_in_kb_per_s() * 1024 / self->time_splice);
      });
    }
  } else {
    if (completion_token_) {
      completion_token_();
    }
  }
}

}
