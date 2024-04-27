#pragma once

#include "op/basic_op.h"

namespace orange {

class AckOp : public BasicOp {
 public:
  AckOp(SessionContext* context);
  ~AckOp() override;

  void do_operation() override;
  std::string name() override;

 private:
};

}