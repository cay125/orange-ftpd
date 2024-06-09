#include "config/config.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <string>

namespace orange {

Config* Config::instance() {
  static Config global_config;
  return &global_config;
}

#define set_field(field_name)                \
  if (json_data.contains(#field_name)) {     \
    field_name##_ = json_data[#field_name];  \
  }

void Config::init(std::string path) {
  std::ifstream conf_file(path);
  if (!conf_file.is_open()) {
    spdlog::critical("Invalid conf path: {}", path);
    return;
  }
  nlohmann::json json_data = nlohmann::json::parse(conf_file);
  set_field(ftpd_banner);
  set_field(max_idle_timeout);
  set_field(data_connection_timeout);
  set_field(data_transfer_port);
  set_field(use_local_time);
  set_field(anonymous_enable);
  set_field(write_enable);
  set_field(port);
  set_field(download_speed_limit_in_kb_per_s);
}

void Config::inc_client_num() {
  client_num_++;
}

void Config::dec_client_num() {
  client_num_--;
}

int Config::client_num() {
  return client_num_;
}

}