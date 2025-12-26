cmake_minimum_required(VERSION 3.15.0)

project(logger LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME}_compiler_flags INTERFACE)
target_compile_features(${PROJECT_NAME}_compiler_flags INTERFACE cxx_std_17)

add_subdirectory(fmt)

add_library(logger STATIC
  "${CMAKE_CURRENT_LIST_DIR}/include/logger.h"
  "${CMAKE_CURRENT_LIST_DIR}/include/console_sink.h"
  "${CMAKE_CURRENT_LIST_DIR}/include/logger_config.h"
  "${CMAKE_CURRENT_LIST_DIR}/include/default_provider.h"
  "${CMAKE_CURRENT_LIST_DIR}/include/desktop_provider.h"
  "${CMAKE_CURRENT_LIST_DIR}/include/message.h"
  "${CMAKE_CURRENT_LIST_DIR}/src/logger.cpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/include/"
    "${CMAKE_CURRENT_LIST_DIR}/fmt/include"
)

target_link_libraries(logger fmt)

target_compile_options(logger PRIVATE
  -Os
  -flto
  -ffunction-sections
  -fdata-sections
  -fno-exceptions
  -nodefaultlibs
)

target_link_options(logger PRIVATE
  -lc
)

target_compile_definitions(logger PRIVATE
  FMT_USE_LOCALE=0
  FMT_BUILTIN_TYPES=0
  FMT_OPTIMIZE_SIZE=1
)
