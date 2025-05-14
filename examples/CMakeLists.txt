# examples/CMakeLists.txt

add_executable(example_repo
    example_repo.cpp
)
# Link against library modules
target_link_libraries(example_repo PRIVATE
    chronograph
    chronograph-repo-utils  
)

add_executable(example_graph_demo
    example_graph_demo.cpp
)
target_link_libraries(example_graph_demo PRIVATE
    chronograph
    chronograph-graph-utils
)

# pick up headers from include/
target_include_directories(example_repo PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
