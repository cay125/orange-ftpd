#pragma once

#include <atomic>
#include <cstdint>
#include <string>

namespace orange {

#define field_ro_guard(name, type, default_value) \
 public:                                          \
  type get_##name() const {                       \
    return name##_;                               \
  }                                               \
 private:                                         \
  type name##_;

class Config {
 public:
  Config(const Config&) = delete;
  Config& operator=(Config&) = delete;

  void init(std::string path);

  static Config* instance();

  void inc_client_num();
  void dec_client_num();
  int client_num();

  field_ro_guard(ftpd_banner, std::string, "");
  field_ro_guard(data_connection_timeout, int, 10);
  field_ro_guard(max_idle_timeout, int, 20);
  field_ro_guard(data_transfer_port, uint16_t, 10087);
  field_ro_guard(use_local_time, bool, true);
  field_ro_guard(anonymous_enable, bool, false);
  field_ro_guard(write_enable, bool, false);
  field_ro_guard(port, uint16_t, 10086);

 private:
  Config() {}
  std::atomic_int client_num_;
};

}