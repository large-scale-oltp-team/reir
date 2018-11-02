cmake_minimum_required(VERSION 3.8)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}")

include(CheckCXXCompilerFlag)

function(append value)
  foreach(variable ${ARGN})
    set(${variable} "${${variable}} ${value}" PARENT_SCOPE)
  endforeach(variable)
endfunction()

set(LLVM_PATH "" CACHE PATH "path for llvm")
message("path: ${LLVM_PATH}")
if (NOT LLVM_PATH OR LLVM_PATH STREQUAL "OFF")
  find_package(LLVM REQUIRED OPTIONS)
  set(LLVM_CONFIG_BIN ${LLVM_CONFIG_PATH})

  if (EXISTS "/usr/bin/clang-6.0")
    message("I use /usr/bin/clang++-6.0 as C++ compiler")
    set(CMAKE_CXX_COMPILER "/usr/bin/clang++-6.0")
    set(LLVM_CONFIG_BIN "/usr/bin/llvm-config-6.0")
  elseif (EXISTS "/usr/local/bin/clang++")
    message("I use /usr/local/bin/clang++ as C++ compiler")
    set(CMAKE_CXX_COMPILER "/usr/local/bin/clang++")
    set(LLVM_CONFIG_BIN "/usr/local/bin/llvm-config")
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  else ()
    message(FATAL_ERROR "Clang is not found. Set -DCMAKE_CXX_COMPILER option.")
  endif ()
else()
  set(LLVM_CONFIG_BIN "${LLVM_PATH}/bin/llvm-config")
  message("use llvm-config of ${LLVM_CONFIG_BIN}")
endif()

execute_process(
    COMMAND
    ${LLVM_CONFIG_BIN} --libs
    OUTPUT_VARIABLE
    LLVM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

execute_process(
    COMMAND
    ${LLVM_CONFIG_BIN} --includedir
    OUTPUT_VARIABLE
    LLVM_INCLUDE_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

execute_process(
    COMMAND
    ${LLVM_CONFIG_BIN} --libdir
    OUTPUT_VARIABLE
    LLVM_LIBRARY_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

execute_process(
    COMMAND
    ${LLVM_CONFIG_BIN} --system-libs
    OUTPUT_VARIABLE
    LLVM_SYSTEM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

execute_process(
    COMMAND
    ${LLVM_CONFIG_BIN} --version
    OUTPUT_VARIABLE
    LLVM_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

execute_process(
    COMMAND
    ${LLVM_CONFIG_BIN} --build-mode
    OUTPUT_VARIABLE
    LLVM_BUILD_TYPE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

execute_process(
    COMMAND
    ${LLVM_CONFIG_BIN} --has-rtti
    OUTPUT_VARIABLE
    LLVM_HAS_RTTI
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

if (LLVM_HAS_RTTI STREQUAL "YES")
else()
    message(FATAL_ERROR "you have to set -DLLVM_PATH with llvm binary with RTTI.")
endif()

set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -ggdb -fno-omit-frame-pointer ")  # DEBUG
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -O2 -ffast-math -ggdb -g -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie")

option(BUILD_TESTS "Builds test programs" ON)
if (BUILD_TESTS) 
  enable_testing()
endif()

check_cxx_compiler_flag("-std=c++11" CXX_SUPPORTS_CXX11)
if (CXX_SUPPORTS_CXX11)
  if (CYGWIN OR MINGW)
    # MinGW and Cygwin are a bit stricter and lack things like
    # 'strdup', 'stricmp', etc in c++11 mode.
    append("-std=gnu++11" CMAKE_CXX_FLAGS)
  else()
    append("-std=c++11" CMAKE_CXX_FLAGS)
  endif()
else()
  message(FATAL_ERROR "Reir requires C++11 support but the '-std=c++11' flag isn't supported.")
endif()

include_directories(src)

find_package(FOEDUS REQUIRED)
find_package(Boost REQUIRED)
find_package(LevelDB REQUIRED)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

message(STATUS "LLVM version    : ${LLVM_VERSION}")
message(STATUS "LLVM includedir : ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM libdir     : ${LLVM_LIBRARY_DIRS}")
message(STATUS "LLVM build type : ${LLVM_BUILD_TYPE}")
message(STATUS "LLVM has rtti   : ${LLVM_HAS_RTTI}")

add_subdirectory(src)
add_subdirectory(tests)

install(EXPORT reir
    FILE reir-config.cmake
    DESTINATION lib/cmake/reir)

