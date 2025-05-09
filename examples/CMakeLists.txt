# examples/CMakeLists.txt

add_executable(example_repo
    example_repo.cpp
)

# Link against library modules
target_link_libraries(example_repo PRIVATE
    chronograph
    chronograph-repo-utils  
)

# pick up headers from include/
target_include_directories(example_repo PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
