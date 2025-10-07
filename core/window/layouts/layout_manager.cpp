#include "layout_manager.h"
#include "../window_base.h"
#include <algorithm>

namespace DearTs {
namespace Core {
namespace Window {

/**
 * LayoutManager构造函数
 */
LayoutManager::LayoutManager()
    : parentWindow_(nullptr) {
}

/**
 * LayoutManager析构函数
 */
LayoutManager::~LayoutManager() {
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
void LayoutManager::addLayout(const std::string& name, std::unique_ptr<LayoutBase> layout) {
    if (layout) {
        layout->setParentWindow(parentWindow_);
        layouts_[name] = std::move(layout);
    }
}

/**
 * 移除布局
 */
void LayoutManager::removeLayout(const std::string& name) {
    layouts_.erase(name);
}

/**
 * 获取布局
 */
LayoutBase* LayoutManager::getLayout(const std::string& name) const {
    auto it = layouts_.find(name);
    return (it != layouts_.end()) ? it->second.get() : nullptr;
}

/**
 * 渲染所有布局
 */
void LayoutManager::renderAll() {
    for (auto& [name, layout] : layouts_) {
        if (layout && layout->isVisible()) {
            layout->render();
        }
    }
}

/**
 * 更新所有布局
 */
void LayoutManager::updateAll(float width, float height) {
    for (auto& [name, layout] : layouts_) {
        if (layout && layout->isVisible()) {
            layout->updateLayout(width, height);
        }
    }
}

/**
 * 处理事件
 */
void LayoutManager::handleEvent(const SDL_Event& event) {
    for (auto& [name, layout] : layouts_) {
        if (layout && layout->isVisible()) {
            layout->handleEvent(event);
        }
    }
}

/**
 * 获取布局数量
 */
size_t LayoutManager::getLayoutCount() const {
    return layouts_.size();
}

/**
 * 清除所有布局
 */
void LayoutManager::clear() {
    layouts_.clear();
}

/**
 * 设置父窗口
 */
void LayoutManager::setParentWindow(WindowBase* window) {
    parentWindow_ = window;
    
    // 更新所有现有布局的父窗口
    for (auto& [name, layout] : layouts_) {
        if (layout) {
            layout->setParentWindow(window);
        }
    }
}

/**
 * 获取父窗口
 */
WindowBase* LayoutManager::getParentWindow() const {
    return parentWindow_;
}

/**
 * 获取所有布局名称
 */
std::vector<std::string> LayoutManager::getLayoutNames() const {
    std::vector<std::string> names;
    names.reserve(layouts_.size());
    
    for (const auto& [name, layout] : layouts_) {
        names.push_back(name);
    }
    
    return names;
}

/**
 * 检查是否存在指定名称的布局
 */
bool LayoutManager::hasLayout(const std::string& name) const {
    return layouts_.find(name) != layouts_.end();
}

/**
 * 设置布局可见性
 */
void LayoutManager::setLayoutVisible(const std::string& name, bool visible) {
    auto it = layouts_.find(name);
    if (it != layouts_.end() && it->second) {
        it->second->setVisible(visible);
    }
}

/**
 * 获取布局可见性
 */
bool LayoutManager::isLayoutVisible(const std::string& name) const {
    auto it = layouts_.find(name);
    if (it != layouts_.end() && it->second) {
        return it->second->isVisible();
    }
    return false;
}

} // namespace Window
} // namespace Core
} // namespace DearTs