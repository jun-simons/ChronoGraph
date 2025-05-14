# Installation

These instructions will get you a working build of ChronoGraph on your system.

## Prerequisites

- **C++ Compiler**  
  - Compiler with C++17 support (GCC 9+ / Clang 9+ / MSVC 2019+)
- **CMake** ≥ 3.14
- **GoogleTest**  
  - With internet access, it will be installed via CMake.  Otherwise:
  - (optional) Bundled as a submodule in `third_party/googletest`  
  - (optional) install system-wide (e.g. `sudo apt install libgtest-dev` on Ubuntu)  
- *(Optional)* **Graphviz**  
  - To render DOT files produced by the examples  


## Building from Source

```bash
git clone https://github.com/<your-org>/ChronoGraph.git
cd ChronoGraph

# Create a build directory
mkdir build && cd build

# Configure & generate build files
cmake ..

# Compile (use all cores)
make -j$(nproc)    # or `cmake --build . -- -j4`
```
---

## Running Tests

After building, to run the full test suite:

```bash
ctest --output-on-failure -V
```