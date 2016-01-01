cmake_minimum_required(VERSION 3.0)

macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "--std=gnu++11 ${CMAKE_CXX_FLAGS}")
    endif ()
  else ()
    set (CMAKE_CXX_STANDARD 11)
    set (CMake_CXX_STANDART_REQUIRED true)
  endif ()
endmacro(use_cxx11)
use_cxx11()

project(scpak CXX)
message(STATUS "project source: " ${PROJECT_SOURCE_DIR})
message(STATUS "project binary: " ${PROJECT_BINARY_DIR})

aux_source_directory(. main_src)
message(STATUS "source files: " ${main_src})

add_executable(scpak ${main_src})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
message(STATUS "executable output path: " ${EXECUTABLE_OUTPUT_PATH})
