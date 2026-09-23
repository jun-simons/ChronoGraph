# tests/cpp/CMakeLists.txt

file(GLOB CPP_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(unit_tests ${CPP_TEST_SOURCES})

target_link_libraries(unit_tests
  PRIVATE
    chronograph   # INTERFACE alias for the core libs
    GTest::gtest_main
)

# Register each TEST() with CTest individually so failures are reported by name
include(GoogleTest)
gtest_discover_tests(unit_tests)
