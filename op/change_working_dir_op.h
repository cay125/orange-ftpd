#pragma once

#include "op/basic_op.h"

namespace orange {

class ChangeWorkingDirOp : public BasicOp {
 public:
  ChangeWorkingDirOp(SessionContext* context);
  ~ChangeWorkingDirOp() override;

  void do_operation() override;
  std::string name() override;

 private:
};

}