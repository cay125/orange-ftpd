#include "session/code.h"
#include <algorithm>
#include <filesystem>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

namespace orange {

std::unordered_map<ret_code, std::string> Response::code_map = {
  {ret_code::data_conn, "Data connection accepted; transfer starting."},
  {ret_code::syst_ok, "UNIX emulated by ftpd"},
  {ret_code::welcome, "Welcome to ftpd."},
  {ret_code::logged_on, "Logged on."},
  {ret_code::transfer_complete, "Transfer complete."},
  {ret_code::pasv_ok, "Entering Passive Mode."},
  {ret_code::password_required, "Password required."},
  {ret_code::syntax_error, "Syntax error, command unrecognized."},
  {ret_code::invalid_seq, "Invalid sequence of commands."},
  {ret_code::not_allowed, "Login or password incorrect."},
};

SharedConstBuffer Response::get_pasv_response(std::string ip, uint16_t port) {
  auto it = code_map.find(ret_code::pasv_ok);
  std::replace(ip.begin(), ip.end(), '.', ',');
  std::ostringstream stream;
  stream << std::to_string(static_cast<int>(ret_code::pasv_ok))
         << " "
         << it->second
         << " ("
         << ip
         << ","
         << std::to_string(port / 256)
         << ","
         << std::to_string(port % 256)
         << ")\r\n";
  return SharedConstBuffer(stream.str());
}

SharedConstBuffer Response::get_code_string(ret_code code) {
  auto it = code_map.find(code);
  if (it == code_map.end()) {
    spdlog::error("Missing required code: {}", (int)code);
    return SharedConstBuffer(std::to_string(static_cast<int>(code)) + "\r\n");
  }
  return SharedConstBuffer(std::to_string(static_cast<int>(code)) + " " + it->second + "\r\n");
}

SharedConstBuffer Response::get_code_string(ret_code code, std::string info) {
  return SharedConstBuffer(std::to_string(static_cast<int>(code)) + " " + info + "\r\n");
}

SharedConstBuffer Response::get_pwd_response(const fs::path* path) {
  std::string ret = "\"" + path->string() + "\" is the current directory.";
  return get_code_string(ret_code::pwd_or_mkdir_ok, ret);
}

SharedConstBuffer Response::get_cwd_response(const fs::path* path) {
  if (path) {
    std::string ret = "\"" + path->string() + "\" is the current directory.";
    return get_code_string(ret_code::cwd_rmdir_dele_rename_ok, "CWD successful. " + ret);
  }
  return get_code_string(ret_code::noperm_filefatal, "Directory not found.");
}

}