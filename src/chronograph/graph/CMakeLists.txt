# src/chronograph/graph/CMakeLists.txt

# Gather all the .cpp files for the core graph module
set(GRAPH_SOURCES
    Graph.cpp
    Snapshot.cpp
    # add any new graph‐related .cpp here
)

# Build a static lib called "chronograph-graph"
add_library(chronograph-graph STATIC ${GRAPH_SOURCES})

# Make sure users can #include <chronograph/graph/Graph.h>
target_include_directories(chronograph-graph
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

add_subdirectory(algorithms)
# Link the algorithms library so Graph consumers see it transitively
target_link_libraries(chronograph-graph
  PUBLIC
    chronograph-graph-algo
)

# If Graph.cpp depends on other internal modules (e.g. a common utils lib),
# link them here. For now, nothing extra:
# target_link_libraries(chronograph-graph PRIVATE chronograph-common)

# Enable consistent C++17 on this target
set_target_properties(chronograph-graph PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
