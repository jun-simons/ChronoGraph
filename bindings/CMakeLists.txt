# bindings/CMakeLists.txt

pybind11_add_module(chronograph_module
    chronograph_py.cpp
)
# set_target_properties(chronograph_module PROPERTIES
#     OUTPUT_NAME "chronograph"
# )

target_link_libraries(chronograph_module
  PRIVATE
    chronograph-graph
    chronograph-graph-algo
    chronograph-graph-utils
    chronograph-repo
)

target_include_directories(chronograph_module
  PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
