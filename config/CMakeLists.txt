file(GLOB srcs *.cc *.h)

add_library(config STATIC ${srcs})
target_link_libraries(config PRIVATE ${third_party})