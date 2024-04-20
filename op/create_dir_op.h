#pragma once

#include "op/basic_op.h"

namespace orange {

class CreateDirOp : public BasicOp {
 public:
  CreateDirOp(SessionContext* context);
  ~CreateDirOp() override;

  void do_operation() override;
  std::string name() override;

 private:
};

}