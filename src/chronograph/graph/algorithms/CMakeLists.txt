# src/chronograph/graph/algorithms/CMakeLists.txt

# Grab all .cpp in this folder
file(GLOB ALGO_SOURCES CONFIGURE_DEPENDS "*.cpp")

add_library(chronograph-graph-algo STATIC ${ALGO_SOURCES})

# Expose the top-level include directory for <chronograph/...> includes
target_include_directories(chronograph-graph-algo
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(chronograph-graph-algo
  PUBLIC
    chronograph-graph
)

set_target_properties(chronograph-graph-algo PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
