## 🛠️ Building IACore

IACore uses **CMake Presets** to manage toolchains and cross-compilation. This ensures that the correct compilers (Clang) and flags (AVX2/SIMD) are used automatically.

### Prerequisites
* CMake 3.28+
* Ninja Build System
* Vcpkg (Environment variable `VCPKG_ROOT` must be set)
* Clang / Clang-CL

### Build Instructions

**1. Configure**
Select the preset for your target platform.
```bash
# List available presets
cmake --list-presets

# Configure for your platform (e.g., windows-x64, linux-arm64, wasm)
cmake --preset windows-x64
```

**2. Build**

```bash
cmake --build --preset windows-x64
```

### Available Presets

|Preset       |Description                   |Toolchain                           |
|-------------|------------------------------|------------------------------------|
|windows-x64  |Windows (Clang-CL)            |CMake/Toolchains/windows-x64.cmake  |
|linux-x64    |Linux (Clang)                 |CMake/Toolchains/linux-x64.cmake    |
|wasm         |WebAssembly (Emscripten)      |CMake/Toolchains/wasm.cmake         |
|windows-arm64|Windows on ARM (Cross-compile)|CMake/Toolchains/windows-arm64.cmake|
|linux-arm64  |Linux on ARM (Cross-compile)  |CMake/Toolchains/linux-arm64.cmake  |
