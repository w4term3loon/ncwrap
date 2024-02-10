# set output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")

# library target
set(ncwrap_src "${CMAKE_CURRENT_SOURCE_DIR}/src/ncwrap_impl.c")
add_library(ncwrap SHARED ${ncwrap_src})
target_include_directories(ncwrap INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(ncwrap Curses)

