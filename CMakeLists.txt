cmake_minimum_required(VERSION 3.14)
project(chronograph VERSION 0.1.0 LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src/chronograph/graph)
add_subdirectory(src/chronograph/repo)
# add_subdirectory(tests)
# add_subdirectory(src/chronograph/common)

# Include public headers
# include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(chronograph INTERFACE)
target_link_libraries(chronograph
  INTERFACE
    chronograph-graph
    chronograph-repo
)

# Core library target
# add_library(chronograph
#     src/Event.cpp
#     src/Node.cpp
#     src/Edge.cpp
#     src/Graph.cpp
#     src/Snapshot.cpp
#     src/Repository.cpp
# )

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

add_executable(test_Graph tests/test_Graph.cpp)
target_link_libraries(test_Graph PRIVATE chronograph GTest::GTest GTest::Main)
add_test(NAME test_Graph COMMAND test_Graph)

add_executable(test_Snapshot tests/test_Snapshot.cpp)
target_link_libraries(test_Snapshot PRIVATE chronograph GTest::GTest GTest::Main)
add_test(NAME test_Snapshot COMMAND test_Snapshot)

add_executable(test_Diff tests/test_Diff.cpp)
target_link_libraries(test_Diff PRIVATE chronograph GTest::GTest GTest::Main)
add_test(NAME test_Diff COMMAND test_Diff)

add_executable(test_Repository tests/test_Repository.cpp)
target_link_libraries(test_Repository PRIVATE chronograph GTest::GTest GTest::Main)
add_test(NAME test_Repository COMMAND test_Repository)

add_executable(test_Merge tests/test_Merge.cpp)
target_link_libraries(test_Merge PRIVATE chronograph GTest::GTest GTest::Main)
add_test(NAME test_Merge COMMAND test_Merge)

# Python bindings (for future addition)
# add_subdirectory(python)
