# src/chronograph/repo/utils/CMakeLists.txt

set(REPO_UTILS_SOURCES
    printUtils.cpp
)

add_library(chronograph-repo-utils STATIC ${REPO_UTILS_SOURCES})

# Expose <chronograph/repo/utils/...> includes
target_include_directories(chronograph-repo-utils
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Needs the core repo library to know CommitGraph
target_link_libraries(chronograph-repo-utils
  PUBLIC
    chronograph-repo
)

set_target_properties(chronograph-repo-utils PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
