#include "config/config.h"
#include "op/auth_op.h"
#include "op/basic_op.h"
#include "session/code.h"
#include "session/context.h"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace orange {

AuthOp::AuthOp(SessionContext* context) :
  BasicOp(context) {
  
}

AuthOp::~AuthOp() {

}

bool AuthOp::validate() {
  if (context_->user().empty()) {
    write_message(Response::get_code_string(ret_code::invalid_seq, "Login with USER first."));
    context_->mutable_password()->clear();
    return false;
  }
  if (Config::instance()->get_anonymous_enable()) {
    std::string user = context_->user();
    std::transform(user.begin(), user.end(), user.begin(), [](char c){return std::tolower(c);});
    if (user == "anonymous") {
      context_->set_is_anonymous(true);
      context_->set_verified(true);
      return true;
    }
  }
  // todo, check user & passwd
  context_->set_is_anonymous(false);
  context_->set_verified(true);
  return true;
}

void AuthOp::do_operation() {
  if (context_->request().command == "USER") {
    *context_->mutable_user() = context_->request().body;
    write_message(Response::get_code_string(ret_code::password_required));
  } else if (context_->request().command == "PASS") {
    *context_->mutable_password() = context_->request().body;
    if (validate()) {
      write_message(Response::get_code_string(ret_code::logged_on));
    }
  } else {
    write_message(Response::get_code_string(ret_code::not_allowed, "Please login with USER and PASS."));
  }
}

std::string AuthOp::name() {
  return "auth_op";
}

}
