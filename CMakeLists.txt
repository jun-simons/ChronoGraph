cmake_minimum_required(VERSION 3.14)
project(chronograph VERSION 0.1.0 LANGUAGES CXX)

add_subdirectory(${PROJECT_SOURCE_DIR}/extern/pybind11)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src/chronograph/graph)
add_subdirectory(src/chronograph/repo)
add_subdirectory(src/chronograph/repo/utils)
add_subdirectory(examples)
add_subdirectory(bindings)

add_library(chronograph INTERFACE)
target_link_libraries(chronograph
  INTERFACE
    chronograph-graph
    chronograph-repo
    chronograph-repo-utils
)

# Specify include directories for consumers
target_include_directories(chronograph INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Enable testing
enable_testing()

# Node: Update commit hash for GTest as needed
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For windows: prevent overriding compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Find GoogleTest for unit tests
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Unit test targets

add_subdirectory(tests)
add_subdirectory(src/chronograph/cli)
