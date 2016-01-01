cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(scpak CXX)
message(STATUS "project source: " ${PROJECT_SOURCE_DIR})
message(STATUS "project binary: " ${PROJECT_BINARY_DIR})

aux_source_directory(. main_src)
message(STATUS "source files: " ${main_src})

add_executable(scpak ${main_src})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
message(STATUS "executable output path: " ${EXECUTABLE_OUTPUT_PATH})
