# src/chronograph/graph/CMakeLists.txt

# Gather all the .cpp files for the core graph module
set(GRAPH_SOURCES
    Graph.cpp
    GraphState.cpp
    Snapshot.cpp
    # add any new graph‐related .cpp here
)

add_library(chronograph-graph STATIC ${GRAPH_SOURCES})

target_include_directories(chronograph-graph
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(chronograph-graph PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)

# Submodules build on top of the core graph library (they link to it, not
# the other way around)
add_subdirectory(algorithms)
add_subdirectory(utils)
