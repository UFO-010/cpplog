CMAKE_MINIMUM_REQUIRED(VERSION 3.15.0)

project(logger_example LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME}_compiler_flags INTERFACE)
target_compile_features(${PROJECT_NAME}_compiler_flags INTERFACE cxx_std_20)

add_subdirectory(../../ logger)

set(SOURCES "${CMAKE_CURRENT_LIST_DIR}/main.cpp")

add_executable(${PROJECT_NAME} ${SOURCES} )

find_library(LOGGER logger PATH_SUFFIXES logger)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${PROJECT_NAME}_compiler_flags
    logger
)
