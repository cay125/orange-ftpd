#pragma once

#include "session/code.h"
#include <string>

namespace orange {

class ftpResponse {
 public:
  ftpResponse() {}
 
  ret_code code;
  std::string msg;
};

}