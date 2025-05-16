CMAKE_MINIMUM_REQUIRED(VERSION 3.18.0)
enable_testing()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

project(logger_test LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake/")

set(SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/test.cpp"
    # "${CMAKE_CURRENT_LIST_DIR}/../src/logger.cpp"
    # "${CMAKE_CURRENT_LIST_DIR}/../include/logger.h"
)

# set(COVERAGE_SOURCES
#     "${CMAKE_CURRENT_LIST_DIR}/../src/logger.cpp"
#     "${CMAKE_CURRENT_LIST_DIR}/../include/logger.h"
# )

add_subdirectory(../ logger)
# add_library(logger STATIC
#     "${CMAKE_CURRENT_LIST_DIR}/../src/logger.cpp"
#     "${CMAKE_CURRENT_LIST_DIR}/../include/logger.h"
# )
get_target_property(COV_SOURCES logger SOURCES)
get_target_property(LOGGER_DIRECTORY logger SOURCE_DIR)

#find_library(LOGGER logger PATH_SUFFIXES logger)


add_executable(format_test ${SOURCES})

include(GoogleTest)
include(FetchContent)

# FetchContent_Declare(
#   googletest
#   # Specify the commit you depend on and update it regularly.
#   URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
# )
# # For Windows: Prevent overriding the parent project's compiler/linker settings
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)

find_package(GTest REQUIRED)

target_link_libraries(format_test PRIVATE
    logger
    # googletest
    GTest::gtest_main
)

gtest_discover_tests(format_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DISCOVERY_TIMEOUT 30
)

add_test(NAME Test COMMAND format_test)

# to check govegare, install gcov and cobertura
# work with gcc


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	include(CodeCoverage)
    include(CheckCXXCompilerFlag)

    set(COVR_INST
        LLVM
        #GCOVR
    )

    target_append_coverage_flags(
        TARGET format_test
        ${COVR_INST}
    )
    target_append_coverage_flags(
        TARGET logger
        ${COVR_INST}
    )

    if(${COVR_INST} STREQUAL LLVM)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            setup_target_for_coverage_llvm(
                EXECUTABLE ctest --test-dir ${CMAKE_BINARY_DIR}
                SOURCES ${COV_SOURCES}
                TARGET format_test
                DEPENDENCIES format_test
                NAME coverage
                HTML
            )
        endif()
    elseif(${COVR_INST} STREQUAL GCOVR)
        setup_target_for_coverage_gcovr(
            EXECUTABLE ctest --test-dir ${CMAKE_BINARY_DIR}
            SOURCES ${COV_SOURCES}
            TARGET format_test
            DEPENDENCIES format_test
            NAME coverage
            COBERTURA SONARQUBE HTML
        )
    endif()

endif()

