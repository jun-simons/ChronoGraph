# src/chronograph/graph/CMakeLists.txt

# Gather all the .cpp files for the core graph module
set(GRAPH_SOURCES
    Graph.cpp
    Snapshot.cpp
    # add any new graph‐related .cpp here
)

add_library(chronograph-graph STATIC ${GRAPH_SOURCES})

target_include_directories(chronograph-graph
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

# --- algorithms submodule ---
add_subdirectory(algorithms)
target_link_libraries(chronograph-graph
  PUBLIC
    chronograph-graph-algo
)

# --- utils submodule ---
add_subdirectory(utils)
target_link_libraries(chronograph-graph
  PUBLIC
    chronograph-graph-utils
)

set_target_properties(chronograph-graph PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
