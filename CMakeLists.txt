cmake_minimum_required(VERSION 3.10)

project(orange_ftpd)

enable_language(CXX)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

message("-- system info: ${CMAKE_SYSTEM}")
message("-- system host processor: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
message("-- system processor: ${CMAKE_SYSTEM_PROCESSOR}")

add_compile_options(
  -Wall
  -Werror
)

find_package(asio REQUIRED)
message(STATUS "Find package: asio, version: ${asio_VERSION}")

find_package(spdlog REQUIRED)
message(STATUS "Find package: spdlog, version: ${spdlog_VERSION}")

find_package(nlohmann_json REQUIRED)
message(STATUS "Find package: nlohmann_json, version: ${nlohmann_json_VERSION}")

set(third_party asio::asio spdlog::spdlog nlohmann_json::nlohmann_json)

include_directories(${CMAKE_INCLUDE_PATH})
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})

add_subdirectory(session)
add_subdirectory(config)
add_subdirectory(utils)
add_subdirectory(op)

add_executable(ftpd main.cc server.cc server.h)
target_link_libraries(ftpd PRIVATE session utils config op ${third_party})

add_custom_target(
  daemon
  COMMAND cd ${CMAKE_BINARY_DIR} && ./ftpd ../config/ftpd.json
  DEPENDS ftpd
)

