#include "config/config.h"
#include "op/list_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"
#include <array>
#include <asio/write.hpp>
#include <cstddef>
#include <spdlog/spdlog.h>
#include <system_error>

namespace orange {

ListOp::ListOp(SessionContext* context) :
  BasicOp(context) {

}

ListOp::~ListOp() {
  if (context_->data_socket() && context_->data_socket()->is_open()) {
    context_->data_socket()->close();
    spdlog::warn("Data socket is not closed when destroy ListOp");
  }
}

static std::array<std::string, 12> month_mapping = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sept", "Oct", "Nov", "Dec"
};

static SharedConstBuffer list_file(const fs::path& path) {
  std::ostringstream stringstream;
  for (auto& it : fs::directory_iterator(path)) {
    switch (it.status().type()) {
     case fs::file_type::directory:
      stringstream << "d";
      break;
     case fs::file_type::block:
      stringstream << "b";
      break;
     case fs::file_type::character:
      stringstream << "c";
      break;
     case fs::file_type::fifo:
      stringstream << "p";
      break;
     case fs::file_type::regular:
      stringstream << "-";
      break;
     case fs::file_type::symlink:
      stringstream << "l";
      break;
     case fs::file_type::socket:
      stringstream << "s";
      break;
     default:
      break;
    }
    fs::perms file_perm = it.status().permissions();
    stringstream << ((file_perm & fs::perms::owner_read) != fs::perms::none ? "r" : "-");
    stringstream << ((file_perm & fs::perms::owner_write) != fs::perms::none ? "w" : "-");
    stringstream << ((file_perm & fs::perms::owner_exec) != fs::perms::none ? "x" : "-");
    stringstream << ((file_perm & fs::perms::group_read) != fs::perms::none ? "r" : "-");
    stringstream << ((file_perm & fs::perms::group_write) != fs::perms::none ? "w" : "-");
    stringstream << ((file_perm & fs::perms::group_exec) != fs::perms::none ? "x" : "-");
    stringstream << ((file_perm & fs::perms::others_read) != fs::perms::none ? "r" : "-");
    stringstream << ((file_perm & fs::perms::others_write) != fs::perms::none ? "w" : "-");
    stringstream << ((file_perm & fs::perms::others_exec) != fs::perms::none ? "x" : "-");
    stringstream << ' ';
    const std::string user_name = "ftpd", group_name = "ftpd";
    stringstream << it.hard_link_count() << " " << user_name << " " << group_name << " ";
    if (fs::is_regular_file(it)) {
      stringstream << it.file_size() << " ";
    } else {
      stringstream << "0 ";
    }

    struct stat file_state{};
    ::stat(it.path().string().c_str(), &file_state);

    tm* tm;
    if (Config::instance()->get_use_local_time()) {
      tm = std::localtime(&file_state.st_mtime);
    } else {
      tm = std::gmtime(&file_state.st_mtime);
    }
    stringstream << month_mapping[tm->tm_mon] << " " << tm->tm_mday << " " << tm->tm_hour << ":" << tm->tm_min << " ";
    stringstream << it.path().filename().string() << "\r\n";
  }
  return SharedConstBuffer(stringstream.str());
}

void ListOp::handle_data_connection() {
  SharedConstBuffer response = list_file(context_->current_path());
  auto self = shared_from_base<ListOp>();
  write_message(Response::get_code_string(ret_code::data_conn), [self, response](){
    asio::async_write(*self->context_->data_socket(), response, [self](std::error_code ec, size_t n){
      self->write_message(Response::get_code_string(ret_code::transfer_complete));
      self->context_->clear_data_socket();
    });
  });
}

void ListOp::do_operation() {
  auto self = shared_from_base<ListOp>();
  fetch_passive_connection<ListOp>([self](){
    self->handle_data_connection();
  });
}

std::string ListOp::name() {
  return "list_op";
}

}