cmake_minimum_required(VERSION 3.15...3.31)
project(chronograph VERSION 0.1.0 LANGUAGES CXX)

option(CHRONOGRAPH_BUILD_TESTS    "Build the C++ and Python test suites" ON)
option(CHRONOGRAPH_BUILD_EXAMPLES "Build the example programs"          ON)
option(CHRONOGRAPH_BUILD_PYTHON   "Build the Python bindings"           ON)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src/chronograph/graph)
add_subdirectory(src/chronograph/repo)
add_subdirectory(src/chronograph/repo/utils)
add_subdirectory(src/chronograph/io)

add_library(chronograph INTERFACE)
target_link_libraries(chronograph
  INTERFACE
    chronograph-graph
    chronograph-graph-algo
    chronograph-graph-utils
    chronograph-repo
    chronograph-repo-utils
    chronograph-io
)

# Specify include directories for consumers
target_include_directories(chronograph INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(CHRONOGRAPH_BUILD_PYTHON)
  # Resolve one interpreter up front so the extension module and the Python
  # tests always agree. Override with -DPython_EXECUTABLE=/path/to/python.
  # On macOS, prefer the interpreter on PATH over Xcode's bundled framework.
  if(NOT DEFINED Python_FIND_FRAMEWORK)
    set(Python_FIND_FRAMEWORK LAST)
  endif()
  set(Python_FIND_VIRTUALENV FIRST)
  find_package(Python 3.8 REQUIRED COMPONENTS Interpreter Development.Module)
  set(PYBIND11_FINDPYTHON ON)
  add_subdirectory(${PROJECT_SOURCE_DIR}/extern/pybind11)
  add_subdirectory(bindings)
endif()

if(CHRONOGRAPH_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(CHRONOGRAPH_BUILD_TESTS)
  enable_testing()

  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  # For windows: prevent overriding compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  add_subdirectory(tests)
endif()
