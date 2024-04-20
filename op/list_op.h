#pragma once

#include "op/basic_op.h"

namespace orange {

class ListOp : public BasicOp {
 public:
  ListOp(SessionContext* context);
  ~ListOp() override;

  void do_operation() override;
  std::string name() override;

 private:
  void handle_data_connection();
};

}