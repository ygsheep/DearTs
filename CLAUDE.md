# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

### Basic Build
```bash
# Create build directory and configure (Windows with Visual Studio)
mkdir build

# Build Debug configuration
mkdir build/Debug && cd build/Debug
cmake -G "Visual Studio 17 2022" -A x64 ../..
cmake --build . --config Debug

# Build Release configuration
mkdir build/Release && cd build/Release
cmake -G "Visual Studio 17 2022" -A x64 ../..
cmake --build . --config Release

```

### Build Options
```bash
# Enable tests
cmake -DDEARTS_BUILD_TESTS=ON ..

# Enable examples
cmake -DDEARTS_BUILD_EXAMPLES=ON ..

# Enable documentation
cmake -DDEARTS_BUILD_DOCS=ON ..

# Enable profiling
cmake -DDEARTS_ENABLE_PROFILING=ON ..

# Disable logging
cmake -DDEARTS_ENABLE_LOGGING=OFF ..
```

### Running the Application
After building, the executable will be located at:
- Debug: `build/Debug/DearTs.exe`
- Release: `build/Release/DearTs.exe`

## Architecture Overview

DearTs is a modern C++ application framework based on SDL2 and ImGui, using a layered, event-driven architecture with plugin support.

### Core Components
- **Application Layer** (`main/`): Entry point and GUI application management
- **Core Library** (`core/`): Core systems and utilities
- **Libdearts Library** (`lib/libdearts/`): Public API and plugin system
- **Plugin System** (`plugins/`): Dynamic plugin loading and management
- **Third-party Integration** (`lib/third_party/`): SDL2, ImGui, and other dependencies

### Key Managers (Singleton Pattern)
- `ApplicationManager`: Application lifecycle management
- `WindowManager`: SDL2 window creation and management
- `EventSystem`: Global event distribution
- `ResourceManager`: Font, image, and other resource management
- `InputManager`: Keyboard, mouse, and controller input
- `AudioManager`: SDL2_mixer audio playback
- `RenderManager`: ImGui rendering integration

### Window System
The window system uses a layout-based architecture:
- `WindowBase`: Base window functionality
- `MainWindow`: Primary application window
- Layout system: Modular UI components (title bar, sidebar, pomodoro timer, etc.)

### Plugin Architecture
- Plugins are dynamic libraries (.dll on Windows)
- Must implement `IPlugin` interface
- Support for dependency management and automatic loading
- Two types: built-in (internal) and external plugins

## Development Guidelines

### Code Style
The project uses `.clang-format` with LLVM-based style:
- 120 character column limit
- 2-space indentation
- Templates break before declarations
- No brace wrapping for functions/classes

### File Organization
```
core/
├── app/           # Application management
├── audio/         # Audio processing
├── events/        # Event system
├── input/         # Input handling
├── patterns/      # Design patterns
├── render/        # Rendering system
├── resource/      # Resource management
├── utils/         # Utility classes
└── window/        # Window management
```

### Memory Management
- Use RAII principles
- Smart pointers preferred over raw pointers
- Automatic resource cleanup through destructors

### Naming Conventions
- Namespaces: `DearTs::Core::Component`
- Classes: PascalCase (`WindowManager`)
- Functions: camelCase (`initialize()`)
- Variables: snake_case (`current_window`)
- Constants: UPPER_CASE (`MAX_WINDOWS`)

## Dependencies

### Core Dependencies
- **SDL2**: Graphics, input, audio, and window management
- **ImGui**: Immediate mode GUI framework
- **SDL2_ttf**: Font rendering support
- **SDL2_image**: Image loading (PNG, JPG, TIFF, WebP)
- **SDL2_mixer**: Audio playback

### Third-party Libraries Location
All third-party libraries are located in `lib/third_party/` with proper include paths and library files for both x86 and x64 architectures.

## Platform Support

### Windows (Primary)
- Visual Studio 2022 recommended
- Automatic DLL copying to build output
- Windows-specific API integration

### Cross-platform Considerations
- CMake handles platform-specific library linking
- Conditional compilation for platform-specific features
- UTF-8 console support on Windows

## Common Development Tasks

### Adding New Windows
1. Inherit from `WindowBase` class
2. Implement required virtual methods
3. Register with `WindowManager`
4. Add to layout system if needed

### Creating Plugins
1. Implement `IPlugin` interface
2. Export required C-style functions
3. Build as dynamic library
4. Place in `plugins/builtin/` or `plugins/external/`

### Event Handling
1. Define event types in `EventSystem`
2. Register event handlers
3. Use event dispatcher for communication

### Resource Management
1. Use `ResourceManager` for fonts and images
2. Resources are automatically loaded and cached
3. Support for custom resource types

## Testing

Tests can be enabled with `DEARTS_BUILD_TESTS=ON` and will be built in the `tests/` directory. Run tests using CTest after building.

## Configuration

The application supports runtime configuration through JSON files. Default configuration is handled by `ConfigManager` singleton.

## Project-Specific Notes

- No test framework currently implemented (CMake option exists but unused)
- Logging controlled via `DEARTS_ENABLE_LOGGING` compile definition
- Profiling instrumentation available when enabled
- Plugin system structure exists but minimal implementation
- Code follows [360 C++ Coding Style](https://saferules.github.io/)
- Private member variables prefixed with `m_` (e.g., `m_window`, `m_title`)
