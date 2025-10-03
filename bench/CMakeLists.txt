CMAKE_MINIMUM_REQUIRED(VERSION 3.15.0)

project(logger_bench LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME}_compiler_flags INTERFACE)
target_compile_features(${PROJECT_NAME}_compiler_flags INTERFACE cxx_std_17)

add_subdirectory(../ logger)

set(SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/logger_bech.cpp"
)

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.16.0
)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
    google_benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.2
)
FetchContent_MakeAvailable(google_benchmark)

add_executable(${PROJECT_NAME} ${SOURCES} )

find_library(LOGGER logger PATH_SUFFIXES logger)

target_link_libraries(${PROJECT_NAME} PUBLIC
    ${PROJECT_NAME}_compiler_flags
    benchmark::benchmark
    benchmark::benchmark_main
    GTest::gtest_main
    logger
)

