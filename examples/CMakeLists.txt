# examples/CMakeLists.txt

add_executable(example_repo
    example_repo.cpp
)
# Link against library modules
target_link_libraries(example_repo PRIVATE
    chronograph
)

add_executable(example_graph_demo
    example_graph_demo.cpp
)
target_link_libraries(example_graph_demo PRIVATE
    chronograph
)

