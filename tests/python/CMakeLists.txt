# tests/python/CMakeLists.txt

if(NOT CHRONOGRAPH_BUILD_PYTHON)
  return()
endif()

# Run pytest with the same interpreter the extension was built for, and put
# the freshly built module first on the import path. pytest's exit code is the
# test result, so any failure (including import errors) fails CTest.
add_test(NAME python_tests
         COMMAND ${Python_EXECUTABLE} -m pytest -q ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(python_tests PROPERTIES
  ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:chronograph_module>"
  TIMEOUT 60
)
