/**
 * @file plugin.cpp
 * @brief 插件系统实现
 */

#include "core/plugin/plugin.h"
#include "core/event/event_bus.h"
#include "core/plugin/plugin_loader.h"
#include "core/plugin/dependency_resolver.h"
#include "liblogger/logger.h"
#include <format>

namespace DearTs::Core::Plugin {

    using namespace DearTs::Core::Event;

// ================ PluginInfo ================

bool PluginInfo::is_api_compatible(const std::string& current_api_version) const {
    // 简单版本比较
    return api_version == current_api_version;
}

// ================ PluginWrapper ================

PluginWrapper::PluginWrapper(std::unique_ptr<IPlugin, PluginDeleter> plugin)
    : m_plugin(std::move(plugin))
    , m_state(PluginState::Unloaded) {
    auto info = m_plugin->get_info();
    LOG_INFO("Created plugin wrapper for: {} v{} by {}",
             info.name, info.version, info.author);
}

PluginWrapper::~PluginWrapper() {
    if (m_state == PluginState::Enabled || m_state == PluginState::Loaded) {
        unload();
    }
}

PluginWrapper::PluginWrapper(PluginWrapper&& other) noexcept
    : m_plugin(std::move(other.m_plugin))
    , m_state(other.m_state)
    , m_error(std::move(other.m_error)) {
    other.m_state = PluginState::Unloaded;
}

PluginWrapper& PluginWrapper::operator=(PluginWrapper&& other) noexcept {
    if (this != &other) {
        if (m_state == PluginState::Enabled || m_state == PluginState::Loaded) {
            unload();
        }
        m_plugin = std::move(other.m_plugin);
        m_state = other.m_state;
        m_error = std::move(other.m_error);
        other.m_state = PluginState::Unloaded;
    }
    return *this;
}

Result<void, std::string> PluginWrapper::load() {
    if (m_state != PluginState::Unloaded) {
        return Result<void, std::string>::err("Plugin already loaded");
    }

    auto info = m_plugin->get_info();
    LOG_INFO("Loading plugin: {}", info.name);

    auto result = m_plugin->on_load();
    if (result.isErr()) {
        m_error = result.error();
        m_state = PluginState::Error;
        LOG_ERROR("Failed to load plugin '{}': {}", info.name, m_error);
        return result;
    }

    m_state = PluginState::Loaded;
    LOG_INFO("Plugin loaded successfully: {}", info.name);
    return Result<void, std::string>::ok();
}

void PluginWrapper::unload() {
    if (m_state == PluginState::Enabled) {
        disable();
    }

    if (m_state == PluginState::Loaded) {
        auto info = m_plugin->get_info();
        LOG_INFO("Unloading plugin: {}", info.name);
        m_plugin->on_unload();
        m_state = PluginState::Unloaded;
    }
}

void PluginWrapper::enable() {
    if (m_state != PluginState::Loaded) {
        LOG_WARN("Cannot enable plugin in state: {}", static_cast<int>(m_state));
        return;
    }

    auto info = m_plugin->get_info();
    LOG_INFO("Enabling plugin: {}", info.name);
    m_plugin->on_enable();
    m_state = PluginState::Enabled;
}

void PluginWrapper::disable() {
    if (m_state != PluginState::Enabled) {
        LOG_WARN("Cannot disable plugin in state: {}", static_cast<int>(m_state));
        return;
    }

    auto info = m_plugin->get_info();
    LOG_INFO("Disabling plugin: {}", info.name);
    m_plugin->on_disable();
    m_state = PluginState::Loaded;
}

// ================ DynamicPluginWrapper ================

DynamicPluginWrapper::DynamicPluginWrapper(
    IPlugin* plugin,
    DestroyPluginFunc destroy_func,
    std::unique_ptr<DynamicLibraryLoader> loader,
    std::string source_path
)
    : m_destroy_func(destroy_func)
    , m_loader(std::move(loader))
    , m_source_path(std::move(source_path))
{
    // 创建带有自定义 deleter 的 unique_ptr 并存储到基类成员
    m_plugin = std::unique_ptr<IPlugin, PluginDeleter>(plugin, PluginDeleter{destroy_func});

    auto info = m_plugin->get_info();
    LOG_INFO("Created dynamic plugin wrapper for: {} from {}", info.name, m_source_path);
}

DynamicPluginWrapper::~DynamicPluginWrapper() {
    unload();
}

void DynamicPluginWrapper::unload() {
    // 先调用基类的 unload（会调用插件的 on_unload）
    PluginWrapper::unload();

    // 在卸载动态库之前，先销毁插件实例
    if (m_plugin) {
        LOG_INFO("Destroying plugin instance before unloading library");
        m_plugin.reset();  // 这会调用 destroy_func
    }

    // 然后卸载动态库
    if (m_loader) {
        LOG_INFO("Unloading dynamic library: {}", m_source_path);
        m_loader->unload();
        m_loader.reset();
    }
}

// ================ PluginManager ================

Result<void, std::string> PluginManager::add_builtin(std::unique_ptr<IPlugin> plugin) {
    auto info = plugin->get_info();

    // 转换为使用 PluginDeleter 的 unique_ptr
    std::unique_ptr<IPlugin, PluginDeleter> plugin_with_deleter(plugin.release(), PluginDeleter{nullptr});

    // 检查是否已存在
    if (m_plugins.find(info.name) != m_plugins.end()) {
        return Result<void, std::string>::err(
            std::format("Plugin '{}' already exists", info.name)
        );
    }

    auto wrapper = std::make_unique<PluginWrapper>(std::move(plugin_with_deleter));
    auto result = wrapper->load();

    if (result.isErr()) {
        return result;
    }

    wrapper->enable();
    m_plugins[info.name] = std::move(wrapper);

    LOG_INFO("Added builtin plugin: {}", info.name);
    
    // 发布插件列表刷新事件
    EventBus::instance().publish(PluginListRefreshEvent{m_plugins.size()});
    return Result<void, std::string>::ok();
}

Result<void, std::string> PluginManager::load_from_file(const std::filesystem::path& path) {
    // 1. 检查文件是否存在
    if (!std::filesystem::exists(path)) {
        return Result<void, std::string>::err(
            std::format("File not found: {}", path.string())
        );
    }

    // 2. 检查文件扩展名
    std::string ext = path.extension().string();
    bool valid_extension = false;

    #ifdef _WIN32
        valid_extension = (ext == ".dll");
    #elif defined(__linux__)
        valid_extension = (ext == ".so");
    #elif defined(__APPLE__)
        valid_extension = (ext == ".dylib");
    #endif

    if (!valid_extension) {
        return Result<void, std::string>::err(
            std::format("Invalid plugin extension: {} (expected: {})",
                ext,
                #ifdef _WIN32
                    ".dll"
                #elif defined(__linux__)
                    ".so"
                #elif defined(__APPLE__)
                    ".dylib"
                #endif
            )
        );
    }

    LOG_INFO("Loading plugin from: {}", path.string());

    // 3. 加载动态库
    auto loader = DynamicLibraryLoader::create();
    auto load_result = loader->load(path);

    if (load_result.isErr()) {
        return Result<void, std::string>::err(
            std::format("Failed to load library: {}", load_result.error())
        );
    }

    // 4. 解析符号：dearts_create_plugin
    auto create_symbol = loader->get_symbol("dearts_create_plugin");
    if (create_symbol.isErr()) {
        return Result<void, std::string>::err(
            std::format("Plugin export 'dearts_create_plugin' not found: {}", create_symbol.error())
        );
    }

    // 5. 解析符号：dearts_destroy_plugin
    auto destroy_symbol = loader->get_symbol("dearts_destroy_plugin");
    if (destroy_symbol.isErr()) {
        return Result<void, std::string>::err(
            std::format("Plugin export 'dearts_destroy_plugin' not found: {}", destroy_symbol.error())
        );
    }

    // 6. 获取函数指针
    auto create_func = reinterpret_cast<CreatePluginFunc>(create_symbol.unwrap());
    auto destroy_func = reinterpret_cast<DestroyPluginFunc>(destroy_symbol.unwrap());

    // 7. 创建插件实例
    IPlugin* plugin_raw = create_func();
    if (!plugin_raw) {
        return Result<void, std::string>::err(
            "Failed to create plugin instance (create_plugin returned null)"
        );
    }

    // 8. 验证插件信息（使用原始指针）
    auto info = plugin_raw->get_info();
    if (info.name.empty()) {
        destroy_func(plugin_raw);  // 清理资源
        return Result<void, std::string>::err("Plugin name cannot be empty");
    }

    // 9. 检查 API 版本兼容性
    const std::string current_api_version = "1.0.0";
    if (!info.is_api_compatible(current_api_version)) {
        destroy_func(plugin_raw);  // 清理资源
        return Result<void, std::string>::err(
            std::format("Plugin API version mismatch: plugin requires {}, current is {}",
                info.api_version, current_api_version)
        );
    }

    // 10. 检查是否已存在
    if (m_plugins.find(info.name) != m_plugins.end()) {
        destroy_func(plugin_raw);  // 清理资源
        return Result<void, std::string>::err(
            std::format("Plugin '{}' already exists", info.name)
        );
    }

    // 12. 创建包装器并加载
    auto wrapper = std::make_unique<DynamicPluginWrapper>(
        plugin_raw,
        destroy_func,
        std::move(loader),
        path.string()
    );

    auto wrapper_load_result = wrapper->load();
    if (wrapper_load_result.isErr()) {
        return Result<void, std::string>::err(
            std::format("Failed to load plugin '{}': {}", info.name, wrapper_load_result.error())
        );
    }

    wrapper->enable();
    m_plugins[info.name] = std::move(wrapper);

    LOG_INFO("Successfully loaded plugin: {} from {}", info.name, path.filename().string());
    
    // 发布插件列表刷新事件
    EventBus::instance().publish(PluginListRefreshEvent{m_plugins.size()});
    return Result<void, std::string>::ok();
}

Result<size_t, std::string> PluginManager::load_from_directory(const std::filesystem::path& directory) {
    // 1. 检查目录是否存在
    if (!std::filesystem::exists(directory)) {
        return Result<size_t, std::string>::err(
            std::format("Directory '{}' does not exist", directory.string())
        );
    }

    // 2. 检查是否为目录
    if (!std::filesystem::is_directory(directory)) {
        return Result<size_t, std::string>::err(
            std::format("Path '{}' is not a directory", directory.string())
        );
    }

    LOG_INFO("Scanning directory for plugins: {}", directory.string());

    size_t count = 0;
    size_t failed = 0;
    std::vector<std::string> errors;

    // 3. 遍历目录（非递归）
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        // 4. 检查文件扩展名（过滤非插件文件）
        std::string ext = entry.path().extension().string();
        bool is_plugin = false;

        #ifdef _WIN32
            is_plugin = (ext == ".dll");
        #elif defined(__linux__)
            is_plugin = (ext == ".so");
        #elif defined(__APPLE__)
            is_plugin = (ext == ".dylib");
        #endif

        if (!is_plugin) {
            continue;
        }

        // 5. 尝试加载插件
        auto result = load_from_file(entry.path());

        if (result.isOk()) {
            count++;
            LOG_INFO("Successfully loaded: {}", entry.path().filename().string());
        } else {
            failed++;
            std::string error = result.error();
            errors.push_back(std::format("{}: {}", entry.path().filename().string(), error));
            LOG_WARN("Failed to load plugin: {} - {}", entry.path().filename().string(), error);
        }
    }

    // 6. 汇总日志
    LOG_INFO("Directory scan complete: {} loaded, {} failed", count, failed);

    if (!errors.empty() && failed > 0) {
        LOG_WARN("Failed plugins:");
        for (const auto& err : errors) {
            LOG_WARN("  - {}", err);
        }
    }

    return Result<size_t, std::string>::ok(count);
}

bool PluginManager::unload(const std::string& name) {
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        LOG_WARN("Plugin '{}' not found", name);
        return false;
    }

    it->second->unload();
    m_plugins.erase(it);
    LOG_INFO("Unloaded plugin: {}", name);
    return true;
}

Result<void, std::string> PluginManager::enable(const std::string& name) {
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return Result<void, std::string>::err(
            std::format("Plugin '{}' not found", name)
        );
    }

    it->second->enable();
    return Result<void, std::string>::ok();
}

Result<void, std::string> PluginManager::disable(const std::string& name) {
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return Result<void, std::string>::err(
            std::format("Plugin '{}' not found", name)
        );
    }

    it->second->disable();
    return Result<void, std::string>::ok();
}

Result<void, std::string> PluginManager::reload(const std::string& name) {
    // TODO: 实现重载
    return Result<void, std::string>::err("Reload not implemented yet");
}

IPlugin* PluginManager::get_plugin(const std::string& name) {
    auto it = m_plugins.find(name);
    if (it != m_plugins.end()) {
        return it->second->get();
    }
    return nullptr;
}

std::vector<PluginInfo> PluginManager::get_all_plugins_info() const {
    std::vector<PluginInfo> infos;
    infos.reserve(m_plugins.size());

    for (const auto& [name, wrapper] : m_plugins) {
        auto* plugin = wrapper->get();
        if (plugin) {
            infos.push_back(plugin->get_info());
        }
    }

    return infos;
}

Result<PluginState, std::string> PluginManager::get_plugin_state(const std::string& name) const {
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return Result<PluginState, std::string>::err(
            std::format("Plugin '{}' not found", name)
        );
    }
    return Result<PluginState, std::string>::ok(it->second->get_state());
}

void PluginManager::clear() {
    LOG_INFO("Clearing all plugins...");
    m_plugins.clear();
}
[[nodiscard]] bool PluginManager::is_plugin_builtin(const std::string& name) const {
    auto it = m_plugins.find(name);
    if (it == m_plugins.end()) {
        return false;  // 插件不存在，返回 false
    }
    
    // 尝试转换为 DynamicPluginWrapper
    // 如果转换成功，说明是动态插件；否则是内置插件
    auto* dynamic_wrapper = dynamic_cast<DynamicPluginWrapper*>(it->second.get());
    return (dynamic_wrapper == nullptr);  // nullptr 表示内置插件
}


// ================ 依赖解析方法 (NEW) ================

void PluginManager::initialize_dependency_config() {
    // 尝试从 ConfigManager 读取依赖解析模式
    // 如果 ConfigManager 未初始化或配置不存在，使用默认值（Lenient）

    // 获取依赖模式配置（默认："lenient"）
    std::string mode_str = "lenient";  // 默认值

    // 注意：这里我们使用静态检查避免在没有 ConfigManager 的情况下编译失败
    // 如果需要 ConfigManager 支持，请在应用初始化时调用此方法
    #ifdef DEARTS_HAS_CONFIG_MANAGER
    // 尝试从配置读取（如果 ConfigManager 可用）
    // mode_str = ConfigManager::instance().get_or<std::string>("plugins.dependency_mode", "lenient");
    #endif

    // 转换字符串到枚举
    if (mode_str == "strict") {
        m_dependency_mode = 1;
        LOG_INFO("Dependency resolution mode: Strict (from config)");
    } else {
        m_dependency_mode = 0;
        LOG_INFO("Dependency resolution mode: Lenient (default)");
    }
}

void PluginManager::set_dependency_mode(DependencyResolutionMode mode) {
    LOG_INFO("Setting dependency resolution mode to: {}",
             (mode == DependencyResolutionMode::Lenient) ? "Lenient" : "Strict");
    m_dependency_mode = (mode == DependencyResolutionMode::Strict) ? 1 : 0;
}

DependencyResolutionMode PluginManager::get_dependency_mode() const {
    return (m_dependency_mode == 1) ? DependencyResolutionMode::Strict : DependencyResolutionMode::Lenient;
}

DependencyResolutionResult PluginManager::get_last_resolution_result() const {
    // 注意：这里返回的是 load_all_with_dependencies() 中使用的静态变量
    // 由于 const 成员函数不能直接返回非静态成员的引用，
    // 我们需要在 load_all_with_dependencies() 中确保静态变量被正确初始化
    // 这里我们返回一个空结果作为默认值
    static DependencyResolutionResult empty_result;
    return empty_result;
}

Result<void, std::string> PluginManager::load_all_with_dependencies() {
    LOG_INFO("Loading all plugins with dependency resolution...");

    // 1. 收集所有插件指针
    std::unordered_map<std::string, IPlugin*> plugin_ptrs;
    for (const auto& [name, wrapper] : m_plugins) {
        auto* plugin = wrapper->get();
        if (plugin) {
            plugin_ptrs[name] = plugin;
        }
    }

    // 2. 解析依赖
    auto mode = (m_dependency_mode == 1) ? DependencyResolutionMode::Strict : DependencyResolutionMode::Lenient;
    auto result = DependencyResolver::resolve(plugin_ptrs, mode);

    // 3. 存储解析结果到静态变量
    static DependencyResolutionResult last_result;
    last_result = std::move(result);

    // 4. 记录解析结果
    if (!last_result.errors.empty()) {
        LOG_WARN("Dependency resolution found {} errors:", last_result.errors.size());
        for (const auto& error : last_result.errors) {
            LOG_WARN("  - {}", error.to_string());
        }
    }

    if (!last_result.disabled_plugins.empty()) {
        LOG_INFO("Plugins disabled due to missing dependencies:");
        for (const auto& name : last_result.disabled_plugins) {
            LOG_INFO("  - {}", name);
        }
    }

    // 5. 检查严重错误（仅严格模式）
    if (!last_result.success && mode == DependencyResolutionMode::Strict) {
        return Result<void, std::string>::err(
            std::format("Dependency resolution failed (strict mode):\n{}",
                       last_result.to_string())
        );
    }

    // 6. 按照计算出的顺序加载插件
    LOG_INFO("Loading plugins in dependency order:");
    for (const auto& entry : last_result.load_order) {
        auto it = m_plugins.find(entry.plugin_name);
        if (it == m_plugins.end()) {
            LOG_WARN("Plugin '{}' not found in wrapper map, skipping", entry.plugin_name);
            continue;
        }

        auto& wrapper = it->second;

        // 跳过已禁用的插件
        if (entry.target_state == PluginState::Disabled) {
            LOG_INFO("Plugin '{}' disabled due to missing dependencies", entry.plugin_name);
            continue;
        }

        // 跳过已加载的插件
        if (wrapper->get_state() != PluginState::Unloaded) {
            LOG_DEBUG("Plugin '{}' already loaded, skipping", entry.plugin_name);
            continue;
        }

        // 加载插件
        auto load_result = wrapper->load();
        if (load_result.isErr()) {
            LOG_ERROR("Failed to load plugin '{}': {}", entry.plugin_name, load_result.error());
            continue;
        }

        // 启用插件
        wrapper->enable();
        LOG_INFO("Loaded and enabled plugin: {} (order: {})",
                 entry.plugin_name, entry.load_order);
    }

    LOG_INFO("Dependency resolution complete: {} plugins loaded successfully",
             last_result.load_order.size());
    return Result<void, std::string>::ok();
}

} // namespace DearTs::Core::Plugin
