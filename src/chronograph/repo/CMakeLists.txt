# src/chronograph/repo/CMakeLists.txt

set(REPO_SOURCES
    Repository.cpp
    # any other repo-specific .cpp
)

add_library(chronograph-repo STATIC ${REPO_SOURCES})
# Graph module needed for repo
target_link_libraries(chronograph-repo
  PUBLIC
    chronograph-graph
)

target_include_directories(chronograph-repo
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(chronograph-repo PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
