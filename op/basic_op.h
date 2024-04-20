#pragma once

#include "session/context.h"
#include "utils/regist_factory.h"
#include <functional>
#include <memory>

namespace orange {

class BasicOp : public std::enable_shared_from_this<BasicOp> {
 public:
  BasicOp(SessionContext* context);
  virtual ~BasicOp();
  virtual void do_operation() = 0;
  virtual std::string name();
  void set_completion_token(std::function<void(void)> fun);
  bool check_data_port();

  template<typename T>
  std::shared_ptr<T> shared_from_base() {
    return std::dynamic_pointer_cast<T>(shared_from_this());
  }

 protected:
  std::function<void(void)> completion_token_;
  SessionContext* context_;
};

}

#define REGISTER_FTPD_OP(derive_type, name) \
  REGISTER_PLUGIN(orange::BasicOp, derive_type, name, orange::SessionContext*);
