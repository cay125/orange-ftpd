find_package(indicators REQUIRED)
message(STATUS "Find package: asio, version: ${indicators_VERSION}")

find_package(argparse REQUIRED)
message(STATUS "Find package: asio, version: ${argparse_VERSION}")

add_executable(benchmark benchmark.cc)
target_link_libraries(benchmark PRIVATE ${third_party} indicators::indicators argparse::argparse)