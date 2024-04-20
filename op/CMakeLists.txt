file(GLOB srcs *.cc *.h)

add_library(op STATIC ${srcs})
target_link_libraries(op PRIVATE ${third_party})