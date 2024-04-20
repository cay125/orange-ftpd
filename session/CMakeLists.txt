file(GLOB srcs *.cc *.h)

add_library(session STATIC ${srcs})
target_link_libraries(session PRIVATE ${third_party})