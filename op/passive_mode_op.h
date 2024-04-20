#pragma once

#include "op/basic_op.h"

namespace orange {

class PassiveModeOp : public BasicOp {
 public:
  PassiveModeOp(SessionContext* context);
  ~PassiveModeOp() override;

  void do_operation() override;
  std::string name() override;

 private:
};

}