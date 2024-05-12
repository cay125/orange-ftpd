#pragma once

#include "op/basic_op.h"

namespace orange {

class SizeOp : public BasicOp {
 public:
  SizeOp(SessionContext* context);
  ~SizeOp() override;

  void do_operation() override;
  std::string name() override;

 private:
};

}