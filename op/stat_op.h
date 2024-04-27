#pragma once

#include "op/basic_op.h"

namespace orange {

class StatOp : public BasicOp {
 public:
  StatOp(SessionContext* context);
  ~StatOp() override;

  void do_operation() override;
  std::string name() override;

 private:
};

}