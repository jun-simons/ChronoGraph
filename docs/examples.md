# Examples

### C++ Examples

The `examples/` directory contains standalone demos showing various ChronoGraph features.

You can run them after building with the included CMake files.

---

```bash
# 1) Build all examples
mkdir -p build && cd build
cmake ..
make -j

# 2) Run the examples
#   – Git-style repo demo:
./examples/example_repo

#   – More complex graph algorithms demo:
./examples/example_graph_demo
```

### Python Examples

Python usage examples are included in the `examples/python/` directory. ChronoGraph must be built and included as a Python extension at `build/bindings/chronograph.so`.