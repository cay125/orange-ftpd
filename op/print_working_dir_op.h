#pragma once

#include "op/basic_op.h"

namespace orange {

class PrintWorkingDirOp : public BasicOp {
 public:
  PrintWorkingDirOp(SessionContext* context);
  ~PrintWorkingDirOp() override;

  void do_operation() override;
  std::string name() override;

 private:
};

}
