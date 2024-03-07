cmake_minimum_required(VERSION 3.16)
project(ncwrap)

# add library
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ncwrap")
# add demo app
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/app")
