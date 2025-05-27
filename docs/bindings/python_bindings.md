# Python Bindings (PyBind11)

ChronoGraph exposes its core C++ API to Python via PyBind11.  This document walks through how to build, install, and use the Python module.

---

## 1. Requirements

- **Python 3.8+** interpreter  
- **CMake 3.14+** with `add_subdirectory(extern/pybind11)` enabled  
- **PyBind11** (added as `extern/pybind11` submodule)  
- Your ChronoGraph C++ libraries already built

Before building, ensure:

```bash
git submodule update --init extern/pybind11
```

## 2. Building the Python Extension

1. In your top‐level `CMakeLists.txt` add:

```cmake
add_subdirectory(extern/pybind11)
add_subdirectory(bindings)
```

2. The `bindings/CMakeLists.txt` uses:

```cmake
pybind11_add_module(chronograph_module
  chronograph_py.cpp
)
set_target_properties(chronograph_module PROPERTIES
  OUTPUT_NAME "chronograph"
)
```

3. From your build directory:

```bash
mkdir -p build && cd build
cmake ..
make -j
```