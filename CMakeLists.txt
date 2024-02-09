cmake_minimum_required(VERSION 3.16)
project(ncwrap)

# set output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# library target
set(ncwrap_src "${CMAKE_SOURCE_DIR}/src/ncwrap_impl.c")
add_library(ncwrap SHARED ${ncwrap_src})
target_include_directories(ncwrap INTERFACE "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(ncwrap Curses)

# binary target
set(app_src "${CMAKE_SOURCE_DIR}/app/app.c")
add_executable(app ${app_src})
target_link_libraries(app ncwrap)
