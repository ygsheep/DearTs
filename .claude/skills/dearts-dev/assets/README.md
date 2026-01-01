# Assets

This directory contains code templates and architecture diagrams for DearTs Framework development.

## 📄 Code Templates

### `app_template.cpp`
Basic application template showing how to create a DearTs application.

**Usage:**
```bash
# Copy template to your project
cp assets/app_template.cpp my_app.cpp
```

**Template includes:**
- Application class setup
- Basic ImGui rendering
- Event handling example
- Configuration management example

### `view_template.cpp`
View template for creating custom ImGui views.

**Template includes:**
- ViewWindow base class
- ImGui UI drawing
- Window state management
- Minimum size configuration

### `plugin_template.cpp`
Plugin template for creating DearTs plugins.

**Template includes:**
- IPlugin interface implementation
- Plugin info structure
- Lifecycle hooks (on_load, on_unload)
- Command and view registration

### `cmake_template.txt`
CMake template for DearTs projects.

**Template includes:**
- SDL3 dependency setup
- ImGui configuration
- DearTs core linking
- Basic target configuration

## 🖼️ Architecture Diagrams

14 high-resolution architecture diagrams (PNG format):

| Diagram | Description |
|---------|-------------|
| `01-diagram.png` | Directory structure |
| `02-diagram.png` | Application lifecycle |
| `03-diagram.png` | Core class relationships |
| `04-diagram.png` | Main loop flow |
| `05-diagram.png` | Module dependencies |
| `06-diagram.png` | Event handling flow |
| `07-diagram.png` | Rendering pipeline |
| `08-diagram.png` | Data flow |
| `09-diagram.png` | Build system |
| `10-diagram.png` | Memory management |
| `11-diagram.png` | Performance monitoring |
| `12-diagram.png` | Error handling |
| `13-diagram.png` | Extension modules |
| `14-diagram.png` | Version roadmap |

**Usage:**
```bash
# View diagram
ls assets/*.png | xargs -I {} echo "Open: {}"
```

## 📋 Quick Start

1. **Create a new app:**
   ```bash
   cp assets/app_template.cpp my_app.cpp
   # Edit my_app.cpp with your logic
   ```

2. **Create a new plugin:**
   ```bash
   cp assets/plugin_template.cpp my_plugin.cpp
   # Edit my_plugin.cpp with your plugin logic
   ```

3. **Create a new view:**
   ```bash
   cp assets/view_template.cpp my_view.cpp
   # Edit my_view.cpp with your view content
   ```

4. **Setup build:**
   ```bash
   cp assets/cmake_template.txt CMakeLists.txt
   # Adjust paths and dependencies
   ```

## 💡 Tips

- Templates are starting points - customize them for your needs
- Diagrams are high-resolution - suitable for presentations
- Keep templates updated with framework changes
- Refer to `references/` for detailed API documentation

## 🔗 Related Resources

- **Core API Manuals**: `../references/config_manager_api.md`, `logger_api.md`, `task_manager_api.md`, `plugin_system_api.md`
- **Plugin Examples**: `../../plugins/builtin/`
- **User Guides**: `../../docs/`
