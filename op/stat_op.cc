#include "op/stat_op.h"
#include "config/config.h"
#include "session/code.h"
#include "session/context.h"
#include <sstream>
#include <string>

namespace orange {

StatOp::StatOp(SessionContext* context) :
  BasicOp(context) {

}

StatOp::~StatOp() {

}

static std::string type_to_string(transport_type type) {
  if (type == transport_type::ascii) {
    return "ASCII";
  }
  if (type == transport_type::binary) {
    return "BINARY";
  }
  return "";
}

void StatOp::do_operation() {
  std::stringstream ss;
  std::string ret_code =  std::to_string(static_cast<int>(ret_code::stat_ok));
  ss << ret_code << "-FTP server status:\n"
     << "  Connected to " << context_->control_socket()->local_endpoint().address().to_string() << "\n"
     << "  Logged in as " << context_->user() << "\n"
     << "  TYPE: " << type_to_string(context_->type()) << "\n"
     << "  Session timeout in seconds is " << Config::instance()->get_max_idle_timeout() << "\n"
     << "  Total client connected num is " << Config::instance()->client_num() << "\n"
     << ret_code << " End of status\r\n";
  SharedConstBuffer buffer(ss.str());
  write_message(buffer);
}

std::string StatOp::name() {
  return "syst_op";
}

}