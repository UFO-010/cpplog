cmake_minimum_required(VERSION 3.15.0)

project(logger LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME}_compiler_flags INTERFACE)
target_compile_features(${PROJECT_NAME}_compiler_flags INTERFACE cxx_std_20)

add_library(logger INTERFACE
  "${CMAKE_CURRENT_LIST_DIR}/include/logger.h"
  "${CMAKE_CURRENT_LIST_DIR}/include/console_sink.h"
  "${CMAKE_CURRENT_LIST_DIR}/include/logger_config.h"
  "${CMAKE_CURRENT_LIST_DIR}/include/default_provider.h"
  "${CMAKE_CURRENT_LIST_DIR}/include/desktop_provider.h"
  "${CMAKE_CURRENT_LIST_DIR}/include/message.h"
)

target_include_directories(${PROJECT_NAME} INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}/include/"
)
