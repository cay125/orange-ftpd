#include "config/config.h"
#include "server.h"
#include "utils/regist_operation.h"
#include <asio/io_context.hpp>
#include <spdlog/spdlog.h>

void print_usage() {
  spdlog::info("Usage: ./ftpd <path_to_conf>");
}

int main(int argc, char** argv) {
  if (argc < 2) {
    spdlog::error("missing required inpout");
    print_usage();
    return -1;
  }

  orange::regist_all_operation();

  orange::Config::instance()->init(argv[1]);
  spdlog::info("ftpd start, listen on: {}", orange::Config::instance()->get_port());
  asio::io_context io_context;
  orange::Server server(&io_context, orange::Config::instance()->get_port());
  server.start();

  io_context.run();

  return 0;
}