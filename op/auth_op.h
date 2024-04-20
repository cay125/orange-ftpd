#pragma once

#include "op/basic_op.h"

namespace orange {

class AuthOp : public BasicOp {
 public:
  AuthOp(SessionContext* context);
  ~AuthOp() override;

  void do_operation() override;
  std::string name() override;

 private:
  bool validate();
};

}