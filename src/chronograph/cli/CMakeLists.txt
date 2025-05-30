# Find CLI11 in extern/cli11 (or install it on system)
add_subdirectory(${CMAKE_SOURCE_DIR}/extern/CLI11)

add_executable(chronograph_cli main.cpp)

target_link_libraries(chronograph_cli
  PRIVATE
    chronograph-graph
    chronograph-repo
    chronograph-repo-utils
    CLI11::CLI11
)

target_include_directories(chronograph_cli
  PRIVATE
    ${CMAKE_SOURCE_DIR}/extern/CLI11/include
)
