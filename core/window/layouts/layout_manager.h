#pragma once

#include "layout_base.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <SDL.h>

// Forward declarations
namespace DearTs {
namespace Core {
namespace Window {
    class WindowBase;
}
}
}

namespace DearTs {
namespace Core {
namespace Window {

/**
 * @brief 布局管理器
 * 统一管理所有布局对象，提供布局的创建、销毁、查找等操作
 */
class LayoutManager {
public:
    /**
     * @brief 构造函数
     */
    LayoutManager();
    
    /**
     * @brief 析构函数
     */
    ~LayoutManager();
    
    /**
     * @brief 获取单例实例
     * @return LayoutManager实例引用
     */
    static LayoutManager& getInstance();
    
    /**
     * @brief 添加布局
     * @param name 布局名称
     * @param layout 布局对象
     */
    void addLayout(const std::string& name, std::unique_ptr<LayoutBase> layout);
    
    /**
     * @brief 移除布局
     * @param name 布局名称
     */
    void removeLayout(const std::string& name);
    
    /**
     * @brief 获取布局
     * @param name 布局名称
     * @return 布局对象指针
     */
    LayoutBase* getLayout(const std::string& name) const;
    
    /**
     * @brief 渲染所有布局
     */
    void renderAll();
    
    /**
     * @brief 更新所有布局
     * @param width 可用宽度
     * @param height 可用高度
     */
    void updateAll(float width, float height);
    
    /**
     * @brief 处理事件
     * @param event SDL事件
     */
    void handleEvent(const SDL_Event& event);
    
    /**
     * @brief 获取布局数量
     * @return 布局数量
     */
    size_t getLayoutCount() const;
    
    /**
     * @brief 清除所有布局
     */
    void clear();
    
    /**
     * @brief 设置父窗口
     * @param window 父窗口
     */
    void setParentWindow(WindowBase* window);
    
    /**
     * @brief 获取父窗口
     * @return 父窗口
     */
    WindowBase* getParentWindow() const;
    
    /**
     * @brief 获取所有布局名称
     * @return 布局名称列表
     */
    std::vector<std::string> getLayoutNames() const;
    
    /**
     * @brief 检查是否存在指定名称的布局
     * @param name 布局名称
     * @return 是否存在
     */
    bool hasLayout(const std::string& name) const;
    
    /**
     * @brief 设置布局可见性
     * @param name 布局名称
     * @param visible 是否可见
     */
    void setLayoutVisible(const std::string& name, bool visible);
    
    /**
     * @brief 获取布局可见性
     * @param name 布局名称
     * @return 是否可见
     */
    bool isLayoutVisible(const std::string& name) const;

private:
    /**
     * @brief 禁止拷贝构造
     */
    LayoutManager(const LayoutManager&) = delete;
    
    /**
     * @brief 禁止赋值操作
     */
    LayoutManager& operator=(const LayoutManager&) = delete;
    
    std::unordered_map<std::string, std::unique_ptr<LayoutBase>> layouts_; ///< 布局映射
    WindowBase* parentWindow_;                                             ///< 父窗口
};

} // namespace Window
} // namespace Core
} // namespace DearTs