
include(CheckCXXCompilerFlag)

# check Address Sanitizer support
set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
set(CMAKE_REQUIRED_LIBRARIES "-fsanitize=address")
check_cxx_source_compiles(
  "int main() { return 0; }"
  ASAN_CXX_SUPPORTED
)
unset(CMAKE_REQUIRED_FLAGS)
unset(CMAKE_REQUIRED_LIBRARIES)


# check Memory Sanitizer support
set(CMAKE_REQUIRED_FLAGS "-fsanitize=memory")
set(CMAKE_REQUIRED_LIBRARIES "-fsanitize=memory")
check_cxx_source_compiles(
  "int main() { return 0; }"
  MSAN_CXX_SUPPORTED
)
unset(CMAKE_REQUIRED_FLAGS)
unset(CMAKE_REQUIRED_LIBRARIES)


# check Leak Sanitizer support
check_cxx_compiler_flag("-fsanitize=leak" LSAN_CXX_SUPPORTED)


# check Undefined behavior Sanitizer support
check_cxx_compiler_flag("-fsanitize=undefined" UBSAN_CXX_SUPPORTED)


# check Thread Sanitizer support
set(CMAKE_REQUIRED_FLAGS "-fsanitize=thread")
set(CMAKE_REQUIRED_LIBRARIES "-fsanitize=thread -pthread")
check_cxx_source_compiles(
  "#include <thread>\nint main() { std::thread t; t.join(); return 0; }"
  TSAN_CXX_SUPPORTED
)
unset(CMAKE_REQUIRED_FLAGS)
unset(CMAKE_REQUIRED_LIBRARIES)


macro(add_address_sanitizer)
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
    check_cxx_compiler_flag(-fsanitize=address HAVE_FLAG_SANITIZER)
    if (HAVE_FLAG_SANITIZER)
        message("Adding -fsanitize=address")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        set(DCMAKE_C_FLAGS "${DCMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
        set(DCMAKE_MODULE_LINKER_FLAGS "${DCMAKE_MODULE_LINKER_FLAGS} -fsanitize=address")
        add_compile_definitions("detect_stack_use_after_return=1:atexit=1:print_stats=1:debug=1;")
    else ()
        message(WARNING "-fsanitize=address unavailable")
    endif ()
endmacro()


macro(add_memory_sanitizer)
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=memory")
    check_cxx_compiler_flag(-fsanitize=memory HAVE_FLAG_SANITIZER)
    if (HAVE_FLAG_SANITIZER)
        check_cxx_compiler_flag(-fsanitize=address HAVE_ADDR_SANITIZER)
        if (HAVE_ADDR_SANITIZER)
            message(WARNING "-fsanitize=memory not allowed with --fsanitize=address")
        else()
            message("Adding -fsanitize=memory")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory -fno-omit-frame-pointer -fsanitize-memory-track-origins=2 -fno-sanitize-memory-use-after-dtor")
            set(DCMAKE_C_FLAGS "${DCMAKE_C_FLAGS} -fsanitize=memory -fno-omit-frame-pointer -fsanitize-memory-track-origins=2 -fno-sanitize-memory-use-after-dtor")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=memory")
            set(DCMAKE_MODULE_LINKER_FLAGS "${DCMAKE_MODULE_LINKER_FLAGS} -fsanitize=memory")
        endif()
    else ()
        message(WARNING "-fsanitize=memory unavailable")
    endif ()
endmacro()


macro(add_leak_sanitizer)
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=leak")
    check_cxx_compiler_flag(-fsanitize=leak HAVE_FLAG_SANITIZER)
    if (HAVE_FLAG_SANITIZER)
        message("Adding -fsanitize=leak")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak -fno-omit-frame-pointer")
        set(DCMAKE_C_FLAGS "${DCMAKE_C_FLAGS} -fsanitize=leak -fno-omit-frame-pointer")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=leak")
        set(DCMAKE_MODULE_LINKER_FLAGS "${DCMAKE_MODULE_LINKER_FLAGS} -fsanitize=leak")
    else ()
        message(WARNING "-fsanitize=leak unavailable")
    endif ()
endmacro()


macro(add_undefined_sanitizer)
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
    check_cxx_compiler_flag(-fsanitize=undefined HAVE_FLAG_SANITIZER)
    if (HAVE_FLAG_SANITIZER)
        message("Adding -fsanitize=undefined")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer -fno-sanitize-recover=all \
            -fsanitize=signed-integer-overflow,null,alignment -fsanitize-trap=alignment -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow")
        set(DCMAKE_C_FLAGS "${DCMAKE_C_FLAGS} -fsanitize=undefined -fno-omit-frame-pointer -fno-sanitize-recover=all \
            -fsanitize=signed-integer-overflow,null,alignment -fsanitize-trap=alignment -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
        set(DCMAKE_MODULE_LINKER_FLAGS "${DCMAKE_MODULE_LINKER_FLAGS} -fsanitize=undefined")
    else ()
        message(WARNING "-fsanitize=undefined unavailable")
    endif ()
endmacro()


macro(add_thread_sanitizer)
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=thread")
    check_cxx_compiler_flag(-fsanitize=thread HAVE_FLAG_SANITIZER)
    if (HAVE_FLAG_SANITIZER)
        check_cxx_compiler_flag(-fsanitize=address HAVE_ADDR_SANITIZER)
        if (HAVE_ADDR_SANITIZER)
            message(WARNING "-fsanitize=thread not allowed with --fsanitize=address")
        else()
            message("Adding -fsanitize=thread")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer")
            set(DCMAKE_C_FLAGS "${DCMAKE_C_FLAGS} -fsanitize=thread -fno-omit-frame-pointer")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
            set(DCMAKE_MODULE_LINKER_FLAGS "${DCMAKE_MODULE_LINKER_FLAGS} -fsanitize=thread")
        endif()
    else ()
        message(WARNING "-fsanitize=thread unavailable")
    endif ()
endmacro()


if(ASAN_CXX_SUPPORTED)
    option(${PROJECT_NAME}_ASAN "Address sanitizer" OFF)
    if(${PROJECT_NAME}_ASAN)
        add_address_sanitizer()
    endif()
else()
    message(STATUS "Address sanitizer not supported")
endif()

if(MSAN_CXX_SUPPORTED)
    option(${PROJECT_NAME}_MSAN "Memory sanitizer" OFF)
    if(${PROJECT_NAME}_MSAN)
        add_memory_sanitizer()
    endif()
else()
    message(STATUS "Memory sanitizer not supported")
endif()

if(LSAN_CXX_SUPPORTED)
    option(${PROJECT_NAME}_LSAN "Leak sanitizer" OFF)
    if(${PROJECT_NAME}_LSAN)
        add_leak_sanitizer()
    endif()
else()
    message(STATUS "Leak sanitizer not supported")
endif()

if(UBSAN_CXX_SUPPORTED)
    option(${PROJECT_NAME}_UBSAN "Undefined behavior sanitizer" OFF)
    if(${PROJECT_NAME}_UBSAN)
        add_undefined_sanitizer()
    endif()
else()
    message(STATUS "Undefined behavior sanitizer not supported")
endif()

if(TSAN_CXX_SUPPORTED)
    option(${PROJECT_NAME}_TSAN "Thread sanitizer" OFF)
    if(${PROJECT_NAME}_TSAN)
        add_thread_sanitizer()
    endif()
else()
     message(STATUS "Thread sanitizer not supported")
endif()


# Prefer use external llvm symbolizer on UNIX
if(UNIX)
    set(SYMBOLIZER_PATH "/usr/bin/llvm-symbolizer")
    add_compile_definitions("symbolize=1:external_symbolizer_path=${SYMBOLIZER_PATH}:verbosity=2")
else()
    add_compile_definitions("symbolize=1:verbosity=2")
endif()
