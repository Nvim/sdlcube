# SDL3 GPU is fun

Not much to see here

## build

### Program

```bash
mkdir build
cmake -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=1
cmake --build build -j$(nproc)
```

### Shaders

Use `glslang` or `glslc` to compile GLSL shaders to SPIR-V, SDL takes care of
the rest

```bash
glslang resources/shaders/src/frag.frag -V -e main -o resources/shaders/compiled/frag.spv
```
