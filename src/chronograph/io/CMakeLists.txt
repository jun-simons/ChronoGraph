# src/chronograph/io/CMakeLists.txt

# JSON support: use an installed nlohmann_json if there is one, else fetch it.
# It's a PRIVATE, header-only dependency: consumers never see it.
find_package(nlohmann_json 3.11 QUIET)
if(NOT nlohmann_json_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

add_library(chronograph-io STATIC
    Serialization.cpp
)

target_include_directories(chronograph-io
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(chronograph-io
  PUBLIC
    chronograph-repo
  PRIVATE
    nlohmann_json::nlohmann_json
)

set_target_properties(chronograph-io PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
