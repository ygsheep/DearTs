# IFLOW.md

This file provides guidance to iFlow Cli when working with code in this repository.

## Build & Development Commands

### Configuration & Build

```
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Debug
```

- Primary build directory: `build/`
- Executable output: `build/Debug/DearTs.exe`
- Windows-specific: SDL2.dll and SDL2_ttf.dll are automatically copied to output directory

### Common CMake Options

- `-DDEARTS_ENABLE_LOGGING=ON` (default: ON)
- `-DDEARTS_ENABLE_PROFILING=ON` (default: OFF)
- `-DDEARTS_BUILD_TESTS=ON` (not yet implemented)

## Architecture Overview

### Layered Structure

```
Application Layer (main/)
├── Core Library (core/)
│   ├── Application Management
│   ├── Event System
│   ├── Input Handling
│   ├── Resource Management
│   └── Window System
├── Plugin System (plugins/)
└── Third-Party Integration (lib/third_party/)
    ├── SDL2 (graphics/input)
    ├── ImGui (UI)
    └── Font Libraries
```

### Key Patterns

1. **Manager Pattern** - Core components organized as singletons:

   - `WindowManager` (window lifecycle)
   - `InputManager` (input handling)
   - `ResourceManager` (asset loading)
   - `ApplicationManager` (application state)

2. **ImGui+SDL2 Integration** - Custom backend implementation:

   - SDL2 handles window/events
   - ImGui provides immediate-mode UI
   - Custom title bar implementation

3. **Resource Pipeline**:
   - Fonts in `resources/fonts/`
   - Automatic copying during build
   - ImGui font atlas configuration

### Critical Conventions

- C++20 standard with MSVC-specific pragmas
- Windows API macros (`WIN32_LEAN_AND_MEAN`, `NOMINMAX`)
- SDL2 dynamic linking (DLLs copied post-build)
- ImGui backend customization in `imgui_impl_sdl2.cpp`

## Project-Specific Notes

- No test framework currently implemented (CMake option exists but unused)
- Logging controlled via `DEARTS_ENABLE_LOGGING` compile definition
- Profiling instrumentation available when enabled
- Plugin system structure exists but minimal implementation
- Code follows [360 C++ Coding Style](https://saferules.github.io/)
- Private member variables prefixed with `m_` (e.g., `m_window`, `m_title`)
