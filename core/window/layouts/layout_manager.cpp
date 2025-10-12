#include "layout_manager.h"
#include "../window_base.h"
#include "../../events/layout_events.h"
#include "../../utils/logger.h"
#include <algorithm>
#include <chrono>
#include <numeric>

namespace DearTs {
namespace Core {
namespace Window {

/**
 * LayoutManager构造函数
 */
LayoutManager::LayoutManager()
    : defaultWindowId_("MainWindow")
    , currentWindowId_("MainWindow")  // 初始化当前窗口ID
    , eventDispatcher_(nullptr)
    , lastUpdateTime_(std::chrono::steady_clock::now()) {
}

/**
 * LayoutManager析构函数
 */
LayoutManager::~LayoutManager() {
    cleanupEventSystem();
    clear();
}

/**
 * 获取LayoutManager单例实例
 */
LayoutManager& LayoutManager::getInstance() {
    static LayoutManager instance;
    return instance;
}

/**
 * 添加布局
 */
void LayoutManager::addLayout(const std::string& name, std::unique_ptr<LayoutBase> layout, const std::string& windowId) {
    if (!layout) return;

    // 确定目标窗口ID
    std::string targetWindowId = windowId.empty() ? getCurrentWindowId() : windowId;

    // 如果目标窗口不存在，创建一个默认的
    if (windowLayouts_.find(targetWindowId) == windowLayouts_.end()) {
        windowLayouts_.emplace(targetWindowId, std::unordered_map<std::string, std::unique_ptr<LayoutBase>>{});
        systemLayoutNames_[targetWindowId] = {"TitleBar", "Sidebar"}; // 默认系统布局
        currentContentLayouts_[targetWindowId] = "";
        lastActiveLayouts_[targetWindowId] = "";
    }

    // 设置父窗口
    auto windowIt = windowContexts_.find(targetWindowId);
    if (windowIt != windowContexts_.end()) {
        layout->setParentWindow(windowIt->second);
    }

    // 添加布局到指定窗口
    windowLayouts_[targetWindowId][name] = std::move(layout);

    DEARTS_LOG_DEBUG("添加布局 " + name + " 到窗口 " + targetWindowId);
}

/**
 * 移除布局
 */
void LayoutManager::removeLayout(const std::string& name) {
    // 在所有窗口中查找并移除布局
    for (auto& [windowId, layouts] : windowLayouts_) {
        layouts.erase(name);
    }
}

/**
 * 获取布局
 */
LayoutBase* LayoutManager::getLayout(const std::string& name, const std::string& windowId) const {
    // 确定目标窗口ID
    std::string targetWindowId = windowId.empty() ? getCurrentWindowId() : windowId;

    auto windowIt = windowLayouts_.find(targetWindowId);
    if (windowIt == windowLayouts_.end()) {
        DEARTS_LOG_WARN("窗口不存在: " + targetWindowId + " (查找布局: " + name + ")");
        return nullptr;
    }

    auto layoutIt = windowIt->second.find(name);
    if (layoutIt != windowIt->second.end()) {
        return layoutIt->second.get();
    }

    // 记录调试信息
    DEARTS_LOG_DEBUG("布局不存在: " + name + " (窗口: " + targetWindowId + ")");
    return nullptr;
}

/**
 * 渲染所有布局
 */
void LayoutManager::renderAll(const std::string& windowId) {
    std::string targetWindowId = windowId.empty() ? getCurrentWindowId() : windowId;

    //std::cout << "[RENDER] LayoutManager::renderAll - 渲染窗口 " << targetWindowId << " 的所有布局 (参数windowId: " << (windowId.empty() ? "空" : windowId) << ")" << std::endl;

    auto windowIt = windowLayouts_.find(targetWindowId);
    if (windowIt == windowLayouts_.end()) {
        DEARTS_LOG_WARN("窗口不存在: " + targetWindowId);
        return;
    }

    const auto& layouts = windowIt->second;
    const auto& systemLayouts = systemLayoutNames_[targetWindowId];

    for (const auto& [name, layout] : layouts) {
        if (layout && layout->isVisible()) {
            // 检查是否为系统布局
            bool isSystemLayout = std::find(systemLayouts.begin(),
                                           systemLayouts.end(),
                                           name) != systemLayouts.end();


            if (isSystemLayout) {
                layout->render();
            }
        } else if (layout && !layout->isVisible()) {
            //std::cout << "[RENDER] LayoutManager::renderAll - 跳过隐藏布局: " << name << " (不可见)" << std::endl;
        } else if (!layout) {
            //std::cout << "[RENDER] LayoutManager::renderAll - 跳过空布局: " << name << std::endl;
        }
    }
}

/**
 * 更新所有布局
 */
void LayoutManager::updateAll(float width, float height, const std::string& windowId) {
    std::string targetWindowId = windowId.empty() ? getCurrentWindowId() : windowId;

    auto windowIt = windowLayouts_.find(targetWindowId);
    if (windowIt == windowLayouts_.end()) {
        return;
    }

    for (auto& [name, layout] : windowIt->second) {
        if (layout && layout->isVisible()) {
            layout->updateLayout(width, height);
        }
    }
}

/**
 * 处理事件
 */
void LayoutManager::handleEvent(const SDL_Event& event, const std::string& windowId) {
    // 确定目标窗口ID
    std::string targetWindowId = windowId.empty() ? getCurrentWindowId() : windowId;

    // 记录事件类型用于调试
    if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION) {
        DEARTS_LOG_INFO("LayoutManager::handleEvent - 处理鼠标事件，类型: " + std::to_string(event.type) + " (窗口: " + targetWindowId + ")");
    }

    auto windowIt = windowLayouts_.find(targetWindowId);
    if (windowIt == windowLayouts_.end()) {
        DEARTS_LOG_WARN("窗口不存在: " + targetWindowId + " (事件处理)");
        return;
    }

    // 按优先级顺序处理事件（系统布局优先）
    std::vector<std::string> layoutOrder = getLayoutsByPriority();

    for (const std::string& layoutName : layoutOrder) {
        auto it = windowIt->second.find(layoutName);
        if (it != windowIt->second.end() && it->second) {
            bool isVisible = it->second->isVisible();
            if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION) {
                DEARTS_LOG_INFO("LayoutManager::handleEvent - 布局: " + layoutName + " (窗口: " + targetWindowId + ") 可见: " + std::string(isVisible ? "是" : "否"));
            }

            if (isVisible) {
                it->second->handleEvent(event);
            }
        }
    }
}

/**
 * 获取布局数量
 */
size_t LayoutManager::getLayoutCount() const {
    size_t totalCount = 0;
    for (const auto& [windowId, layouts] : windowLayouts_) {
        totalCount += layouts.size();
    }
    return totalCount;
}

/**
 * 清除所有布局
 */
void LayoutManager::clear() {
    windowLayouts_.clear();
    windowContexts_.clear();
    currentContentLayouts_.clear();
    lastActiveLayouts_.clear();
    systemLayoutNames_.clear();
    messageHandlers_.clear();
}


/**
 * 获取所有布局名称
 */
std::vector<std::string> LayoutManager::getLayoutNames() const {
    std::vector<std::string> names;

    for (const auto& [windowId, layouts] : windowLayouts_) {
        for (const auto& [name, layout] : layouts) {
            names.push_back(name);
        }
    }

    return names;
}

/**
 * 检查是否存在指定名称的布局
 */
bool LayoutManager::hasLayout(const std::string& name) const {
    for (const auto& [windowId, layouts] : windowLayouts_) {
        if (layouts.find(name) != layouts.end()) {
            return true;
        }
    }
    return false;
}

/**
 * 设置布局可见性
 */
void LayoutManager::setLayoutVisible(const std::string& name, bool visible) {
    for (auto& [windowId, layouts] : windowLayouts_) {
        auto it = layouts.find(name);
        if (it != layouts.end() && it->second) {
            it->second->setVisible(visible);
            return; // 找到并设置后退出
        }
    }
}

/**
 * 获取布局可见性
 */
bool LayoutManager::isLayoutVisible(const std::string& name) const {
    for (const auto& [windowId, layouts] : windowLayouts_) {
        auto it = layouts.find(name);
        if (it != layouts.end() && it->second) {
            return it->second->isVisible();
        }
    }
    return false;
}

/**
 * 切换布局显示（隐藏其他布局，只显示指定布局）
 */
bool LayoutManager::switchToLayout(const std::string& layoutName, bool animated) {
    // 检查目标布局是否存在
    if (!hasLayout(layoutName)) {
        DEARTS_LOG_ERROR("切换布局失败，布局不存在: " + layoutName);
        return false;
    }

    // 找到布局所属的窗口
    std::string layoutWindowId = getLayoutWindowId(layoutName);
    if (layoutWindowId.empty()) {
        DEARTS_LOG_ERROR("无法确定布局所属窗口: " + layoutName);
        return false;
    }

    std::string previousLayout = currentContentLayouts_[layoutWindowId];

    // 隐藏所有内容布局（保留系统布局）
    hideAllContentLayouts();

    // 显示目标布局
    if (showLayout(layoutName, "切换布局")) {
        currentContentLayouts_[layoutWindowId] = layoutName;
        DEARTS_LOG_INFO("布局切换成功: " + previousLayout + " -> " + layoutName);
        return true;
    }

    return false;
}

/**
 * 显示布局（保持其他布局状态）
 */
bool LayoutManager::showLayout(const std::string& layoutName, const std::string& reason) {
    for (auto& [windowId, layouts] : windowLayouts_) {
        auto it = layouts.find(layoutName);
        if (it != layouts.end() && it->second) {
            it->second->setVisible(true);
            DEARTS_LOG_INFO("显示布局: " + layoutName + (reason.empty() ? "" : " 原因: " + reason));
            return true;
        }
    }

    DEARTS_LOG_ERROR("显示布局失败，布局不存在: " + layoutName);
    return false;
}

/**
 * 隐藏布局
 */
bool LayoutManager::hideLayout(const std::string& layoutName, const std::string& reason) {
    for (auto& [windowId, layouts] : windowLayouts_) {
        auto it = layouts.find(layoutName);
        if (it != layouts.end() && it->second) {
            it->second->setVisible(false);

            // 如果隐藏的是当前内容布局，清空记录
            if (currentContentLayouts_[windowId] == layoutName) {
                currentContentLayouts_[windowId].clear();
            }

            DEARTS_LOG_INFO("隐藏布局: " + layoutName + (reason.empty() ? "" : " 原因: " + reason));
            return true;
        }
    }

    DEARTS_LOG_ERROR("隐藏布局失败，布局不存在: " + layoutName);
    return false;
}

/**
 * 隐藏所有内容布局（保留系统布局如标题栏、侧边栏）
 */
void LayoutManager::hideAllContentLayouts() {
    DEARTS_LOG_DEBUG("隐藏所有内容布局");

    for (auto& [windowId, layouts] : windowLayouts_) {
        const auto& systemLayouts = systemLayoutNames_[windowId];

        for (auto& [name, layout] : layouts) {
            if (layout) {
                // 检查是否为系统布局（标题栏、侧边栏等）
                bool isSystemLayout = std::find(systemLayouts.begin(),
                                               systemLayouts.end(),
                                               name) != systemLayouts.end();

                if (!isSystemLayout && layout->isVisible()) {
                    layout->setVisible(false);
                    DEARTS_LOG_DEBUG("隐藏内容布局: " + name);
                }
            }
        }

        currentContentLayouts_[windowId].clear();
    }
}


/**
 * 初始化事件系统
 */
void LayoutManager::initializeEventSystem() {
    if (eventDispatcher_) {
        DEARTS_LOG_WARN("事件系统已初始化");
        return;
    }

    // 初始化系统布局名称
    systemLayoutNames_[defaultWindowId_] = {"TitleBar", "Sidebar"};

    // 创建事件调度器
    eventDispatcher_ = new Events::LayoutEventDispatcher();

    // 订阅布局相关事件
    using namespace Events;

    // 订阅布局显示请求
    eventDispatcher_->subscribe(LayoutEventType::LAYOUT_SHOW_REQUEST,
        [this](const LayoutEvent& event) -> bool {
            try {
                const auto& data = std::get<LayoutVisibilityData>(event.getEventData());
                return showLayout(data.layoutName, data.reason.has_value() ? *data.reason : "");
            } catch (const std::bad_variant_access&) {
                // 处理简单字符串数据
                const auto& layoutName = std::get<std::string>(event.getEventData());
                return showLayout(layoutName, "事件请求");
            }
        });

    // 订阅布局隐藏请求
    eventDispatcher_->subscribe(LayoutEventType::LAYOUT_HIDE_REQUEST,
        [this](const LayoutEvent& event) -> bool {
            try {
                const auto& data = std::get<LayoutVisibilityData>(event.getEventData());
                return hideLayout(data.layoutName, data.reason.has_value() ? *data.reason : "");
            } catch (const std::bad_variant_access&) {
                // 处理简单字符串数据
                const auto& layoutName = std::get<std::string>(event.getEventData());
                return hideLayout(layoutName, "事件请求");
            }
        });

    // 订阅布局切换请求
    eventDispatcher_->subscribe(LayoutEventType::LAYOUT_SWITCH_REQUEST,
        [this](const LayoutEvent& event) -> bool {
            try {
                const auto& data = std::get<LayoutSwitchData>(event.getEventData());
                return switchToLayout(data.toLayout, data.animated);
            } catch (const std::bad_variant_access&) {
                // 处理简单字符串数据
                const auto& layoutName = std::get<std::string>(event.getEventData());
                return switchToLayout(layoutName, true);
            }
        });

    DEARTS_LOG_INFO("布局管理器事件系统初始化完成");
}

/**
 * 清理事件系统
 */
void LayoutManager::cleanupEventSystem() {
    if (eventDispatcher_) {
        eventDispatcher_->clear();
        delete eventDispatcher_;
        eventDispatcher_ = nullptr;
        DEARTS_LOG_INFO("布局管理器事件系统已清理");
    }
}

// === 布局注册机制实现 ===

bool LayoutManager::registerLayout(const LayoutRegistration& registration) {
    if (registration.name.empty() || !registration.factory) {
        DEARTS_LOG_ERROR("布局注册失败：名称为空或工厂函数无效");
        return false;
    }

    if (registeredLayouts_.find(registration.name) != registeredLayouts_.end()) {
        DEARTS_LOG_WARN("布局已注册，将被覆盖: " + registration.name);
    }

    registeredLayouts_[registration.name] = registration;

    // 如果设置了自动创建且布局不存在，则立即创建
    if (registration.autoCreate && !hasLayout(registration.name)) {
        createRegisteredLayout(registration.name);
    }

    DEARTS_LOG_INFO("布局注册成功: " + registration.name + " (类型: " +
                    std::to_string(static_cast<int>(registration.type)) +
                    ", 优先级: " + std::to_string(static_cast<int>(registration.priority)) + ")");
    return true;
}

void LayoutManager::unregisterLayout(const std::string& layoutName) {
    auto it = registeredLayouts_.find(layoutName);
    if (it != registeredLayouts_.end()) {
        // 移除布局实例
        removeLayout(layoutName);

        // 移除元数据
        layoutMetadata_.erase(layoutName);

        // 移除注册信息
        registeredLayouts_.erase(it);

        DEARTS_LOG_INFO("布局取消注册: " + layoutName);
    }
}

bool LayoutManager::isLayoutRegistered(const std::string& layoutName) const {
    return registeredLayouts_.find(layoutName) != registeredLayouts_.end();
}

bool LayoutManager::createRegisteredLayout(const std::string& layoutName) {
    auto it = registeredLayouts_.find(layoutName);
    if (it == registeredLayouts_.end()) {
        DEARTS_LOG_ERROR("布局未注册: " + layoutName);
        return false;
    }

    if (hasLayout(layoutName)) {
        DEARTS_LOG_WARN("布局实例已存在: " + layoutName);
        return true;
    }

    try {
        auto layout = it->second.factory();
        if (!layout) {
            DEARTS_LOG_ERROR("布局工厂函数返回空指针: " + layoutName);
            return false;
        }

        std::string currentWindowId = getCurrentWindowId();
        DEARTS_LOG_DEBUG("创建布局 " + layoutName + " 并添加到窗口: " + currentWindowId);
        addLayout(layoutName, std::move(layout), currentWindowId);

        // 初始化元数据
        layoutMetadata_[layoutName] = LayoutMetadata{};

        DEARTS_LOG_INFO("布局实例创建成功: " + layoutName + " (窗口: " + currentWindowId + ")");
        return true;
    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("创建布局实例失败: " + layoutName + " 错误: " + e.what());
        return false;
    }
}

std::vector<std::string> LayoutManager::getRegisteredLayoutNames() const {
    std::vector<std::string> names;
    names.reserve(registeredLayouts_.size());

    for (const auto& [name, registration] : registeredLayouts_) {
        names.push_back(name);
    }

    return names;
}

// === 布局优先级管理实现 ===

bool LayoutManager::setLayoutPriority(const std::string& layoutName, LayoutPriority priority) {
    auto it = registeredLayouts_.find(layoutName);
    if (it == registeredLayouts_.end()) {
        DEARTS_LOG_ERROR("布局未注册，无法设置优先级: " + layoutName);
        return false;
    }

    LayoutPriority oldPriority = it->second.priority;
    it->second.priority = priority;

    DEARTS_LOG_INFO("布局优先级更新: " + layoutName + " " +
                    std::to_string(static_cast<int>(oldPriority)) + " -> " +
                    std::to_string(static_cast<int>(priority)));
    return true;
}

LayoutPriority LayoutManager::getLayoutPriority(const std::string& layoutName) const {
    auto it = registeredLayouts_.find(layoutName);
    if (it != registeredLayouts_.end()) {
        return it->second.priority;
    }
    return LayoutPriority::NORMAL;
}

std::vector<std::string> LayoutManager::getLayoutsByPriority() const {
    std::vector<std::pair<std::string, LayoutPriority>> layoutPriorities;

    for (const auto& [windowId, layouts] : windowLayouts_) {
        for (const auto& [name, layout] : layouts) {
            if (layout) {
                LayoutPriority priority = getLayoutPriority(name);
                layoutPriorities.emplace_back(name, priority);
            }
        }
    }

    // 按优先级从高到低排序
    std::sort(layoutPriorities.begin(), layoutPriorities.end(),
              [](const auto& a, const auto& b) {
                  return static_cast<int>(a.second) > static_cast<int>(b.second);
              });

    std::vector<std::string> result;
    result.reserve(layoutPriorities.size());
    for (const auto& [name, priority] : layoutPriorities) {
        result.push_back(name);
    }

    return result;
}

// === 布局依赖关系管理实现 ===

bool LayoutManager::checkLayoutDependencies(const std::string& layoutName) const {
    auto it = registeredLayouts_.find(layoutName);
    if (it == registeredLayouts_.end()) {
        return true; // 未注册的布局认为无依赖
    }

    for (const std::string& dependency : it->second.dependencies) {
        if (!hasLayout(dependency) || !isLayoutVisible(dependency)) {
            return false;
        }
    }

    return true;
}

std::vector<std::string> LayoutManager::getLayoutDependencies(const std::string& layoutName) const {
    auto it = registeredLayouts_.find(layoutName);
    if (it == registeredLayouts_.end()) {
        return {};
    }

    return std::vector<std::string>(it->second.dependencies.begin(),
                                   it->second.dependencies.end());
}

bool LayoutManager::addLayoutDependency(const std::string& layoutName, const std::string& dependency) {
    auto it = registeredLayouts_.find(layoutName);
    if (it == registeredLayouts_.end()) {
        DEARTS_LOG_ERROR("布局未注册，无法添加依赖: " + layoutName);
        return false;
    }

    it->second.dependencies.insert(dependency);
    DEARTS_LOG_INFO("添加布局依赖: " + layoutName + " -> " + dependency);
    return true;
}

bool LayoutManager::removeLayoutDependency(const std::string& layoutName, const std::string& dependency) {
    auto it = registeredLayouts_.find(layoutName);
    if (it == registeredLayouts_.end()) {
        DEARTS_LOG_ERROR("布局未注册，无法移除依赖: " + layoutName);
        return false;
    }

    size_t removed = it->second.dependencies.erase(dependency);
    if (removed > 0) {
        DEARTS_LOG_INFO("移除布局依赖: " + layoutName + " -> " + dependency);
        return true;
    }
    return false;
}

// === 布局状态管理实现 ===

bool LayoutManager::setLayoutState(const std::string& layoutName, LayoutState state) {
    auto it = layoutMetadata_.find(layoutName);
    if (it == layoutMetadata_.end()) {
        DEARTS_LOG_ERROR("布局元数据不存在: " + layoutName);
        return false;
    }

    LayoutState oldState = it->second.state;
    it->second.state = state;
    it->second.lastActive = std::chrono::steady_clock::now();
    it->second.isDirty = true;

    if (state == LayoutState::ACTIVE || state == LayoutState::VISIBLE || state == LayoutState::FOCUSED) {
        // 找到布局所属的窗口并更新最后激活布局
        std::string layoutWindowId = getLayoutWindowId(layoutName);
        if (!layoutWindowId.empty()) {
            lastActiveLayouts_[layoutWindowId] = layoutName;
        }
    }

    DEARTS_LOG_DEBUG("布局状态更新: " + layoutName + " " +
                     std::to_string(static_cast<int>(oldState)) + " -> " +
                     std::to_string(static_cast<int>(state)));
    return true;
}

LayoutState LayoutManager::getLayoutState(const std::string& layoutName) const {
    auto it = layoutMetadata_.find(layoutName);
    if (it != layoutMetadata_.end()) {
        return it->second.state;
    }
    return LayoutState::INACTIVE;
}

std::vector<std::string> LayoutManager::getLayoutsByState(LayoutState state) const {
    std::vector<std::string> result;

    for (const auto& [name, metadata] : layoutMetadata_) {
        if (metadata.state == state) {
            result.push_back(name);
        }
    }

    return result;
}

// === 布局元数据管理实现 ===

bool LayoutManager::setLayoutMetadata(const std::string& layoutName, const std::string& key, const std::string& value) {
    auto it = layoutMetadata_.find(layoutName);
    if (it == layoutMetadata_.end()) {
        DEARTS_LOG_ERROR("布局元数据不存在: " + layoutName);
        return false;
    }

    it->second.customData[key] = value;
    it->second.isDirty = true;
    return true;
}

std::string LayoutManager::getLayoutMetadata(const std::string& layoutName, const std::string& key) const {
    auto it = layoutMetadata_.find(layoutName);
    if (it != layoutMetadata_.end()) {
        auto keyIt = it->second.customData.find(key);
        if (keyIt != it->second.customData.end()) {
            return keyIt->second;
        }
    }
    return "";
}

void LayoutManager::markLayoutDirty(const std::string& layoutName, bool dirty) {
    auto it = layoutMetadata_.find(layoutName);
    if (it != layoutMetadata_.end()) {
        it->second.isDirty = dirty;
    }
}

bool LayoutManager::isLayoutDirty(const std::string& layoutName) const {
    auto it = layoutMetadata_.find(layoutName);
    return it != layoutMetadata_.end() && it->second.isDirty;
}

// === 布局生命周期管理实现 ===

bool LayoutManager::activateLayout(const std::string& layoutName) {
    if (!hasLayout(layoutName)) {
        if (isLayoutRegistered(layoutName)) {
            if (!createRegisteredLayout(layoutName)) {
                return false;
            }
        } else {
            DEARTS_LOG_ERROR("布局不存在且未注册: " + layoutName);
            return false;
        }
    }

    // 检查依赖
    if (!checkLayoutDependencies(layoutName)) {
        DEARTS_LOG_ERROR("布局依赖不满足: " + layoutName);
        return false;
    }

    // 解决冲突
    std::string layoutWindowId = getLayoutWindowId(layoutName);
    if (!resolveLayoutConflicts(layoutName, layoutWindowId)) {
        DEARTS_LOG_ERROR("无法解决布局冲突: " + layoutName);
        return false;
    }

    // 激活布局
    setLayoutState(layoutName, LayoutState::ACTIVE);
    showLayout(layoutName, "激活布局");

    // 更新最后激活布局
    if (!layoutWindowId.empty()) {
        lastActiveLayouts_[layoutWindowId] = layoutName;
    }
    DEARTS_LOG_INFO("布局激活成功: " + layoutName);
    return true;
}

bool LayoutManager::deactivateLayout(const std::string& layoutName) {
    if (!hasLayout(layoutName)) {
        DEARTS_LOG_WARN("尝试停用不存在的布局: " + layoutName);
        return false;
    }

    setLayoutState(layoutName, LayoutState::INACTIVE);
    hideLayout(layoutName, "停用布局");

    DEARTS_LOG_INFO("布局停用成功: " + layoutName);
    return true;
}

std::string LayoutManager::getLastActiveLayout() const {
    // 返回默认窗口的最后激活布局
    return lastActiveLayouts_.empty() ? "" : lastActiveLayouts_.at(defaultWindowId_);
}

bool LayoutManager::resolveLayoutConflicts(const std::string& layoutName, const std::string& windowId) {
    auto it = registeredLayouts_.find(layoutName);
    if (it == registeredLayouts_.end()) {
        return true; // 未注册的布局认为无冲突
    }

    // 隐藏冲突的布局
    for (const std::string& conflict : it->second.conflicts) {
        if (hasLayout(conflict) && isLayoutVisible(conflict)) {
            DEARTS_LOG_INFO("解决布局冲突: 隐藏 " + conflict + " 以激活 " + layoutName);
            hideLayout(conflict, "布局冲突解决");
        }
    }

    return true;
}

// === 窗口上下文管理实现 ===

void LayoutManager::registerWindowContext(const std::string& windowId, WindowBase* window) {
    windowContexts_[windowId] = window;

    // 初始化窗口的布局数据
    if (windowLayouts_.find(windowId) == windowLayouts_.end()) {
        windowLayouts_.emplace(windowId, std::unordered_map<std::string, std::unique_ptr<LayoutBase>>{});
        systemLayoutNames_[windowId] = {"TitleBar", "Sidebar"};
        currentContentLayouts_[windowId] = "";
        lastActiveLayouts_[windowId] = "";
    }

    DEARTS_LOG_DEBUG("注册窗口上下文: " + windowId);
}

void LayoutManager::unregisterWindowContext(const std::string& windowId) {
    windowContexts_.erase(windowId);
    windowLayouts_.erase(windowId);
    systemLayoutNames_.erase(windowId);
    currentContentLayouts_.erase(windowId);
    lastActiveLayouts_.erase(windowId);

    DEARTS_LOG_DEBUG("注销窗口上下文: " + windowId);
}

LayoutBase* LayoutManager::getWindowLayout(const std::string& windowId, const std::string& layoutName) const {
    auto windowIt = windowLayouts_.find(windowId);
    if (windowIt == windowLayouts_.end()) {
        return nullptr;
    }

    auto layoutIt = windowIt->second.find(layoutName);
    if (layoutIt != windowIt->second.end()) {
        return layoutIt->second.get();
    }

    return nullptr;
}

std::vector<std::string> LayoutManager::getRegisteredWindowIds() const {
    std::vector<std::string> windowIds;
    windowIds.reserve(windowContexts_.size());

    for (const auto& [windowId, window] : windowContexts_) {
        windowIds.push_back(windowId);
    }

    return windowIds;
}

std::string LayoutManager::getCurrentContentLayout() const {
    // 获取当前活跃窗口的当前内容布局
    std::string currentWindowId = getCurrentWindowId();
    auto it = currentContentLayouts_.find(currentWindowId);
    return (it == currentContentLayouts_.end()) ? "" : it->second;
}

// === 辅助方法实现 ===

std::string LayoutManager::getCurrentWindowId() const {
    // 返回当前活跃窗口ID；如果未设置则返回默认窗口ID
    return currentWindowId_.empty() ? defaultWindowId_ : currentWindowId_;
}

void LayoutManager::setActiveWindow(const std::string& windowId) {
    if (windowId != currentWindowId_) {
        std::string previousWindow = currentWindowId_;
        currentWindowId_ = windowId.empty() ? defaultWindowId_ : windowId;

        DEARTS_LOG_DEBUG("活跃窗口切换: " + previousWindow + " -> " + currentWindowId_);
    }
}

std::string LayoutManager::getLayoutWindowId(const std::string& layoutName) const {
    // 在所有窗口中查找布局
    for (const auto& [windowId, layouts] : windowLayouts_) {
        if (layouts.find(layoutName) != layouts.end()) {
            return windowId;
        }
    }
    return "";
}

// === 布局间通信机制实现 ===

bool LayoutManager::sendLayoutMessage(const std::string& fromWindowId, const std::string& fromLayoutName,
                                     const std::string& toWindowId, const std::string& toLayoutName,
                                     const std::string& message) {
    // 如果指定了目标布局，只发送给该布局
    if (!toLayoutName.empty()) {
        LayoutBase* targetLayout = getWindowLayout(toWindowId, toLayoutName);
        if (targetLayout) {
            // 这里可以调用布局的消息处理方法
            DEARTS_LOG_DEBUG("发送布局消息: " + fromWindowId + ":" + fromLayoutName +
                            " -> " + toWindowId + ":" + toLayoutName + " : " + message);
            return true;
        }
    } else {
        // 发送给目标窗口的所有布局
        auto windowIt = windowLayouts_.find(toWindowId);
        if (windowIt != windowLayouts_.end()) {
            for (const auto& [layoutName, layout] : windowIt->second) {
                if (layout) {
                    DEARTS_LOG_DEBUG("广播布局消息到: " + toWindowId + ":" + layoutName +
                                    " : " + message);
                }
            }
            return true;
        }
    }

    return false;
}

void LayoutManager::registerLayoutMessageHandler(const std::string& windowId, const std::string& layoutName,
                                                 std::function<void(const std::string&, const std::string&, const std::string&)> handler) {
    messageHandlers_[windowId][layoutName] = handler;
    DEARTS_LOG_DEBUG("注册布局消息处理器: " + windowId + ":" + layoutName);
}

void LayoutManager::broadcastMessage(const std::string& fromWindowId, const std::string& fromLayoutName,
                                    const std::string& message) {
    // 广播给所有窗口的所有布局
    for (const auto& [windowId, layouts] : windowLayouts_) {
        for (const auto& [layoutName, layout] : layouts) {
            if (layout) {
                auto handlerIt = messageHandlers_[windowId].find(layoutName);
                if (handlerIt != messageHandlers_[windowId].end()) {
                    handlerIt->second(fromWindowId, fromLayoutName, message);
                }
            }
        }
    }

    DEARTS_LOG_DEBUG("广播布局消息: " + fromWindowId + ":" + fromLayoutName + " -> all : " + message);
}

// === 父窗口管理实现 ===

void LayoutManager::setParentWindow(WindowBase* window, const std::string& windowId) {
    std::string targetWindowId = windowId.empty() ? getCurrentWindowId() : windowId;

    // 注册窗口上下文
    registerWindowContext(targetWindowId, window);

    // 设置为活跃窗口
    setActiveWindow(targetWindowId);

    // 为该窗口的所有现有布局设置父窗口
    auto windowIt = windowLayouts_.find(targetWindowId);
    if (windowIt != windowLayouts_.end()) {
        for (auto& [name, layout] : windowIt->second) {
            if (layout) {
                layout->setParentWindow(window);
            }
        }
    }

    DEARTS_LOG_DEBUG("设置父窗口: " + targetWindowId + " (已设为活跃窗口)");
}

} // namespace Window
} // namespace Core
} // namespace DearTs