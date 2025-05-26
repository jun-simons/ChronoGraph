# tests/python/CMakeLists.txt

# 1) Locate Python3
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# 2) Compute the path to your built bindings
#    we assume your build tree is alongside your source tree
#    and that the module is in build/bindings/chronograph.so
set(BINDINGS_DIR "${CMAKE_BINARY_DIR}/bindings")

# 3) Register a CTest that runs pytest via python -m pytest
add_test(NAME python_tests
         COMMAND ${Python3_EXECUTABLE}
                 -c 
                   "import sys; sys.path.insert(0, '${BINDINGS_DIR}'); \
                    import pytest; \
                    pytest.main(['-q','${CMAKE_CURRENT_SOURCE_DIR}'])"
)

# 4) (Optional) increase timeout if your Python tests take longer
set_tests_properties(python_tests PROPERTIES TIMEOUT 60)
