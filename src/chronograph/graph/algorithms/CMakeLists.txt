# src/chronograph/graph/algorithms/CMakeLists.txt

# Grab all .cpp in this folder
file(GLOB ALGO_SOURCES "*.cpp")

add_library(chronograph-graph-algo STATIC ${ALGO_SOURCES})

# Expose the top-level include directory for <chronograph/...> includes
target_include_directories(chronograph-graph-algo
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# It doesn’t depend on anything else in Graph besides the headers,
# so no extra target_link_libraries() needed.

set_target_properties(chronograph-graph-algo PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
