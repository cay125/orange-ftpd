#pragma once

#include <string>

namespace orange {

struct FtpRequest {
  std::string command;
  std::string body;

  void reset() {
    command.clear();
    body.clear();
  }
};

}