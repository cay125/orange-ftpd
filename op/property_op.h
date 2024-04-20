#pragma once

#include "op/basic_op.h"

namespace orange {

class PropertyOp : public BasicOp {
 public:
  PropertyOp(SessionContext* context);
  ~PropertyOp() override;

  void do_operation() override;
  std::string name() override;

 private:
};

}