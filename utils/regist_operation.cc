#include "op/ack_op.h"
#include "op/auth_op.h"
#include "op/basic_op.h"
#include "op/change_working_dir_op.h"
#include "op/create_dir_op.h"
#include "op/list_op.h"
#include "op/passive_mode_op.h"
#include "op/print_working_dir_op.h"
#include "op/property_op.h"
#include "op/recv_file_op.h"
#include "op/send_file_op.h"
#include "op/stat_op.h"
#include "op/syst_op.h"
#include "utils/regist_operation.h"
#include <spdlog/spdlog.h>

namespace orange {

void regist_all_operation() {
  REGISTER_FTPD_OP(AckOp, NOOP)
  REGISTER_FTPD_OP(AuthOp, AUTH);
  REGISTER_FTPD_OP(CreateDirOp, MKD);
  REGISTER_FTPD_OP(CreateDirOp, XMKD);
  REGISTER_FTPD_OP(SendFileOp, RETR);
  REGISTER_FTPD_OP(RecvFileOp, STOR);
  REGISTER_FTPD_OP(StatOp, STAT);
  REGISTER_FTPD_OP(SystOp, SYST);
  REGISTER_FTPD_OP(PrintWorkingDirOp, PWD);
  REGISTER_FTPD_OP(PrintWorkingDirOp, XPWD);
  REGISTER_FTPD_OP(ChangeWorkingDirOp, CWD);
  REGISTER_FTPD_OP(ChangeWorkingDirOp, XCWD);
  REGISTER_FTPD_OP(PropertyOp, OPTS);
  REGISTER_FTPD_OP(PropertyOp, TYPE);
  REGISTER_FTPD_OP(PassiveModeOp, PASV);
  REGISTER_FTPD_OP(ListOp, LIST);

  auto& op_map = PluginRegistry<orange::BasicOp, orange::SessionContext*>::getFinalFactoryMap();
  spdlog::info("Total registed ops: {}", op_map.size());
}

}