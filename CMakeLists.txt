cmake_minimum_required(VERSION 3.15.0)

project(logger LANGUAGES C CXX)

option(${PROJECT_NAME}_SHARED "Shared lib" OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME}_compiler_flags INTERFACE)
target_compile_features(${PROJECT_NAME}_compiler_flags INTERFACE cxx_std_17)

add_library(logger INTERFACE
  # "${CMAKE_CURRENT_LIST_DIR}/src/logger.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/include/logger.h"
  "${CMAKE_CURRENT_LIST_DIR}/include/console_sink.h"
  "${CMAKE_CURRENT_LIST_DIR}/include/stream_logger.h"
  "${CMAKE_CURRENT_LIST_DIR}/include/default_provider.h"
  "${CMAKE_CURRENT_LIST_DIR}/include/logger_config.h"
)

target_include_directories(${PROJECT_NAME} INTERFACE
    # "${CMAKE_CURRENT_LIST_DIR}/src/"
    "${CMAKE_CURRENT_LIST_DIR}/include/"
)

# target_compile_options(${PROJECT_NAME} PRIVATE
#     -ffunction-sections
#     -fdata-sections
#     -fno-exceptions
#     -fstrict-aliasing
#     -Wpedantic
#     -Wall
#     -Wextra
#     -Wshadow
#     -Wformat=2
#     -Wunused
#     -Werror
#     -Wno-deprecated
#     -Wwrite-strings
#     -Wunused-variable
#     -Wunused-parameter
#     -Wunused-function
#     -Wcast-align
#     -Wuninitialized
# )
