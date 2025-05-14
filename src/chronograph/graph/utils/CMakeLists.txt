# src/chronograph/graph/algorithms/utils/CMakeLists.txt

# Collect our implementation file(s)
set(VISUAL_SOURCES
    Visual.cpp
)

add_library(chronograph-graph-utils STATIC ${VISUAL_SOURCES})

# Make <chronograph/...> includes resolve from top-level include/
target_include_directories(chronograph-graph-utils
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Depend on the core graph library for Graph.h, getNodes(), etc.
target_link_libraries(chronograph-graph-utils
  PUBLIC
    chronograph-graph
)

set_target_properties(chronograph-graph-utils PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)