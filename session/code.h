#pragma once

#include <asio/buffer.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace orange {

enum class ret_code {
  data_conn = 150,
  ok = 200,
  stat_ok = 211,
  size_or_mdtm_ok = 213,
  syst_ok = 215,
  welcome = 220,
  transfer_complete = 226,
  pasv_ok = 227,
  logged_on = 230,
  cwd_rmdir_dele_rename_ok = 250,
  pwd_or_mkdir_ok = 257,
  password_required = 331,
  bad_send_net = 426,
  syntax_error = 500,
  bad_opts = 501,
  invalid_seq = 503,
  not_allowed = 530,
  noperm_filefatal = 550,
  upload_fail = 553,
};

// A reference-counted non-modifiable buffer class.
class SharedConstBuffer
{
public:
  // Construct from a std::string.
  explicit SharedConstBuffer(const std::string& data)
    : data_(new std::vector<char>(data.begin(), data.end())),
      buffer_(asio::buffer(*data_)) {
  }

  SharedConstBuffer() {
  }

  auto count() const {
    return data_.use_count();
  }

  bool valid() {
    return data_.get() != nullptr;
  }

  // Implement the ConstBufferSequence requirements.
  typedef asio::const_buffer value_type;
  typedef const asio::const_buffer* const_iterator;
  const asio::const_buffer* begin() const { return &buffer_; }
  const asio::const_buffer* end() const { return &buffer_ + 1; }

private:
  std::shared_ptr<std::vector<char> > data_;
  asio::const_buffer buffer_;
};

class Response {
 public:
  static SharedConstBuffer get_code_string(ret_code code);
  static SharedConstBuffer get_code_string(ret_code code, std::string info);
  static SharedConstBuffer get_pasv_response(std::string ip, uint16_t port);
  static SharedConstBuffer get_pwd_response(const fs::path* path);
  static SharedConstBuffer get_cwd_response(const fs::path* path);
 private:
  Response() {}
  static std::unordered_map<ret_code, std::string> code_map;
};

}
