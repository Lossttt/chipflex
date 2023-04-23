cmake_minimum_required(VERSION 3.21)

project(chipflex C)

add_executable(${PROJECT_NAME} src/main.c)
target_include_directories(${PROJECT_NAME} PUBLIC include/)

configure_file(options.json ${CMAKE_BINARY_DIR} COPYONLY)


