# set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

# binary target
set(app_src "${CMAKE_CURRENT_SOURCE_DIR}/app.c")
add_executable(app ${app_src})
target_link_libraries(app ncwrap)

