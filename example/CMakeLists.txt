CMAKE_MINIMUM_REQUIRED(VERSION 3.18.0)

project(logger_example LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake/")
include(Sanitize)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# specify the C++ standard
add_library(${PROJECT_NAME}_compiler_flags INTERFACE)
add_library(${PROJECT_NAME}_compiler_flags_c INTERFACE)
add_library(${PROJECT_NAME}_compiler_flags_cxx INTERFACE)

target_link_libraries(${PROJECT_NAME}_compiler_flags INTERFACE ${PROJECT_NAME}_compiler_flags_c)
target_link_libraries(${PROJECT_NAME}_compiler_flags INTERFACE ${PROJECT_NAME}_compiler_flags_cxx)
target_compile_features(${PROJECT_NAME}_compiler_flags INTERFACE cxx_std_17)


add_subdirectory(../ logger)

if(WIN32)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	set(BUILD_SHARED_LIBS OFF)
	set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

set(SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/main.cpp"
)


add_executable(${PROJECT_NAME} ${SOURCES} )

find_library(LOGGER logger PATH_SUFFIXES logger)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${PROJECT_NAME}_compiler_flags
    logger
)

set(gcc_like_c "$<COMPILE_LANG_AND_ID:C,ARMClang,AppleClang,Clang,GNU,LCC>")
target_compile_options(${PROJECT_NAME}_compiler_flags_c INTERFACE
  "$<${gcc_like_c}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wno-system-headers;-Wno-deprecated;-Wwrite-strings;-Wunused;-Wunused-variable;-Wunused-parameter;-Wunused-function;-Wcast-align;-Wpedantic;-Werror;-ffunction-sections;-fdata-sections>>"

  # Set build parameters:
  #         Debug
  #         Release
  #         MinSizeRel
  #         RelWithDebInfo
  "$<$<CONFIG:Debug>:$<${gcc_like_c}:$<BUILD_INTERFACE:-O0;-DDEBUG;>>>"
  "$<$<CONFIG:Release>:$<${gcc_like_c}:$<BUILD_INTERFACE:-O2;-DNDEBUG;>>>"
  "$<$<CONFIG:MinSizeRel>:$<${gcc_like_c}:$<BUILD_INTERFACE:-Os;-DNDEBUG;>>>"
  "$<$<CONFIG:RelWithDebInfo>:$<${gcc_like_c}:$<BUILD_INTERFACE:-O2;-DNDEBUG;>>>"
  )


set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
target_compile_options(${PROJECT_NAME}_compiler_flags_cxx INTERFACE
  "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wattributes;-Wshadow;-Wno-system-headers;-Wno-deprecated;-Woverloaded-virtual;-Wwrite-strings;-Wunused;-Wunused-variable;-Wunused-parameter;-Wunused-function;-Wcast-align;-Wold-style-cast;-Wpedantic;-Wuninitialized;-ffunction-sections;-fdata-sections;>>"

  # Set build parameters:
  #         Debug
  #         Release
  #         MinSizeRel
  #         RelWithDebInfo
  "$<$<CONFIG:Debug>:$<${gcc_like_cxx}:$<BUILD_INTERFACE:-O0;-DDEBUG;>>>"
  "$<$<CONFIG:Release>:$<${gcc_like_cxx}:$<BUILD_INTERFACE:-O2;-DNDEBUG;>>>"
  "$<$<CONFIG:MinSizeRel>:$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Os;-DNDEBUG;>>>"
  "$<$<CONFIG:RelWithDebInfo>:$<${gcc_like_cxx}:$<BUILD_INTERFACE:-O2;-DNDEBUG;>>>"
)
