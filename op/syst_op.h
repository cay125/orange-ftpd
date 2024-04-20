#pragma once

#include "op/basic_op.h"

namespace orange {

class SystOp : public BasicOp {
 public:
  SystOp(SessionContext* context);
  ~SystOp() override;

  void do_operation() override;
  std::string name() override;

 private:
};

}