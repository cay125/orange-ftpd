file(GLOB srcs *.cc *.h)

add_library(utils STATIC ${srcs})
target_link_libraries(utils PRIVATE ${third_party})