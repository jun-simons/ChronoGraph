# tests/cpp/CMakeLists.txt

file(GLOB CPP_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(unit_tests ${CPP_TEST_SOURCES})

target_link_libraries(unit_tests
  PRIVATE
    chronograph   # INTERFACE alias for the core libs
    GTest::GTest
    GTest::Main
)

# 3) Register a single CTest test that runs ALL TEST()s
enable_testing()
add_test(
  NAME unit_tests
  COMMAND unit_tests
)
