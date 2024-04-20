#include "session/context.h"
#include <cstdlib>
#include <filesystem>

namespace orange {

SessionContext::SessionContext() :
  current_path_(fs::path(std::getenv("HOME"))),
  verified_(false),
  enable_write_(false),
  is_anonymous_(false),
  type_(transport_type::binary) {
}

}