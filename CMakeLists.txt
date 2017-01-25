cmake_minimum_required(VERSION 3.1)

set (CMAKE_CXX_STANDARD 11)
set (CMake_CXX_STANDART_REQUIRED true)

project(scpak CXX)
message(STATUS "project source: " ${PROJECT_SOURCE_DIR})
message(STATUS "project binary: " ${PROJECT_BINARY_DIR})

aux_source_directory(. scpak_src)
file(GLOB scpak_src ${scpak_src} "tinyxml2/tinyxml2.cpp")
message(STATUS "scpak source files: " ${scpak_src})

add_executable(scpak ${scpak_src})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
message(STATUS "executable output: " ${EXECUTABLE_OUTPUT_PATH})
