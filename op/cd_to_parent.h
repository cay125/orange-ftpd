#pragma once

#include "op/basic_op.h"
namespace orange {

class CdToParentOp : public BasicOp {
 public:
  CdToParentOp(SessionContext* context);
  ~CdToParentOp() override;

  void do_operation() override;
  std::string name() override;

 private:
};


}