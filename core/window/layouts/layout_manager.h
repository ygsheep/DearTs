#pragma once

#include "layout_base.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <functional>
#include <variant>
#include <optional>
#include <chrono>
#include <SDL.h>

// Forward declarations
namespace DearTs {
namespace Core {
namespace Window {
    class WindowBase;
}
namespace Events {
    class LayoutEvent;
    class LayoutEventDispatcher;
}
}
}

namespace DearTs {
namespace Core {
namespace Window {

/**
 * @brief 布局类型枚举
 */
enum class LayoutType {
    SYSTEM,         ///< 系统布局（标题栏、侧边栏等）
    CONTENT,        ///< 内容布局（主要功能区域）
    MODAL,          ///< 模态布局（对话框、弹出窗口等）
    UTILITY,        ///< 工具布局（工具栏、状态栏等）
    OVERLAY         ///< 覆盖层布局（通知、提示等）
};

/**
 * @brief 布局优先级
 */
enum class LayoutPriority {
    LOWEST = 0,     ///< 最低优先级
    LOW = 25,       ///< 低优先级
    NORMAL = 50,    ///< 普通优先级
    HIGH = 75,      ///< 高优先级
    HIGHEST = 100   ///< 最高优先级
};

/**
 * @brief 布局状态枚举
 */
enum class LayoutState {
    INACTIVE,       ///< 未激活
    ACTIVE,         ///< 已激活
    VISIBLE,        ///< 可见
    FOCUSED,        ///< 获得焦点
    MODAL           ///< 模态状态
};

/**
 * @brief 布局注册信息
 */
struct LayoutRegistration {
    std::string name;                       ///< 布局名称
    LayoutType type;                        ///< 布局类型
    LayoutPriority priority;                ///< 优先级
    std::set<std::string> dependencies;     ///< 依赖的其他布局
    std::set<std::string> conflicts;        ///< 冲突的布局
    std::function<std::unique_ptr<LayoutBase>()> factory; ///< 布局工厂函数
    bool autoCreate = true;                 ///< 是否自动创建
    bool persistent = false;                ///< 是否持久化状态

    LayoutRegistration() = default;

    LayoutRegistration(const std::string& n, LayoutType t, LayoutPriority p)
        : name(n), type(t), priority(p) {}
};

/**
 * @brief 布局元数据
 */
struct LayoutMetadata {
    LayoutState state = LayoutState::INACTIVE;  ///< 当前状态
    std::string lastFocused;                     ///< 最后获得焦点的布局
    std::chrono::steady_clock::time_point lastActive; ///< 最后激活时间
    std::unordered_map<std::string, std::string> customData; ///< 自定义数据
    bool isDirty = false;                        ///< 是否需要保存
};

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
     * @param windowId 窗口ID（可选，默认为当前父窗口）
     */
    void addLayout(const std::string& name, std::unique_ptr<LayoutBase> layout, const std::string& windowId = "");
    
    /**
     * @brief 移除布局
     * @param name 布局名称
     */
    void removeLayout(const std::string& name);
    
    /**
     * @brief 获取布局
     * @param name 布局名称
     * @param windowId 窗口ID（可选，为空则在当前活跃窗口中查找）
     * @return 布局对象指针
     */
    LayoutBase* getLayout(const std::string& name, const std::string& windowId = "") const;
    
    /**
     * @brief 渲染所有布局
     * @param windowId 窗口ID（可选，指定窗口则只渲染该窗口的布局）
     */
    void renderAll(const std::string& windowId = "");

    /**
     * @brief 更新所有布局
     * @param width 可用宽度
     * @param height 可用高度
     * @param windowId 窗口ID（可选，指定窗口则只更新该窗口的布局）
     */
    void updateAll(float width, float height, const std::string& windowId = "");
    
    /**
     * @brief 处理事件
     * @param event SDL事件
     * @param windowId 窗口ID（可选，为空则在当前活跃窗口中处理）
     */
    void handleEvent(const SDL_Event& event, const std::string& windowId = "");
    
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
     * @param windowId 窗口ID（可选，默认使用窗口标题）
     */
    void setParentWindow(WindowBase* window, const std::string& windowId = "");

    /**
     * @brief 获取父窗口
     * @param windowId 窗口ID
     * @return 父窗口
     */
    WindowBase* getParentWindow(const std::string& windowId = "") const;

    /**
     * @brief 注册窗口上下文
     * @param windowId 窗口ID
     * @param window 窗口指针
     */
    void registerWindowContext(const std::string& windowId, WindowBase* window);

    /**
     * @brief 注销窗口上下文
     * @param windowId 窗口ID
     */
    void unregisterWindowContext(const std::string& windowId);

    /**
     * @brief 获取窗口的布局
     * @param windowId 窗口ID
     * @param layoutName 布局名称
     * @return 布局对象指针
     */
    LayoutBase* getWindowLayout(const std::string& windowId, const std::string& layoutName) const;
    
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

    /**
     * @brief 切换布局显示（隐藏其他布局，只显示指定布局）
     * @param layoutName 要显示的布局名称
     * @param animated 是否使用动画过渡
     * @return 是否成功切换
     */
    bool switchToLayout(const std::string& layoutName, bool animated = true);

    /**
     * @brief 显示布局（保持其他布局状态）
     * @param layoutName 布局名称
     * @param reason 显示原因
     * @return 是否成功
     */
    bool showLayout(const std::string& layoutName, const std::string& reason = "");

    /**
     * @brief 隐藏布局
     * @param layoutName 布局名称
     * @param reason 隐藏原因
     * @return 是否成功
     */
    bool hideLayout(const std::string& layoutName, const std::string& reason = "");

    /**
     * @brief 隐藏所有内容布局（保留系统布局如标题栏、侧边栏）
     */
    void hideAllContentLayouts();

    /**
     * @brief 获取当前可见的内容布局名称
     * @return 布局名称，如果没有则返回空字符串
     */
    std::string getCurrentContentLayout() const;

    /**
     * @brief 初始化事件系统
     * 订阅布局相关事件
     */
    void initializeEventSystem();

    /**
     * @brief 清理事件系统
     * 取消订阅所有事件
     */
    void cleanupEventSystem();

    // === 布局注册机制 ===

    /**
     * @brief 注册布局类型
     * @param registration 布局注册信息
     * @return 是否注册成功
     */
    bool registerLayout(const LayoutRegistration& registration);

    /**
     * @brief 取消注册布局类型
     * @param layoutName 布局名称
     */
    void unregisterLayout(const std::string& layoutName);

    /**
     * @brief 检查布局是否已注册
     * @param layoutName 布局名称
     * @return 是否已注册
     */
    bool isLayoutRegistered(const std::string& layoutName) const;

    /**
     * @brief 创建已注册的布局实例
     * @param layoutName 布局名称
     * @return 是否创建成功
     */
    bool createRegisteredLayout(const std::string& layoutName);

    /**
     * @brief 获取所有已注册的布局名称
     * @return 布局名称列表
     */
    std::vector<std::string> getRegisteredLayoutNames() const;

    // === 布局优先级管理 ===

    /**
     * @brief 设置布局优先级
     * @param layoutName 布局名称
     * @param priority 优先级
     * @return 是否设置成功
     */
    bool setLayoutPriority(const std::string& layoutName, LayoutPriority priority);

    /**
     * @brief 获取布局优先级
     * @param layoutName 布局名称
     * @return 优先级，如果不存在则返回NORMAL
     */
    LayoutPriority getLayoutPriority(const std::string& layoutName) const;

    /**
     * @brief 按优先级获取布局名称列表（从高到低）
     * @return 排序后的布局名称列表
     */
    std::vector<std::string> getLayoutsByPriority() const;

    // === 布局依赖关系管理 ===

    /**
     * @brief 检查布局依赖是否满足
     * @param layoutName 布局名称
     * @return 依赖是否满足
     */
    bool checkLayoutDependencies(const std::string& layoutName) const;

    /**
     * @brief 获取布局的依赖列表
     * @param layoutName 布局名称
     * @return 依赖的布局名称列表
     */
    std::vector<std::string> getLayoutDependencies(const std::string& layoutName) const;

    /**
     * @brief 添加布局依赖
     * @param layoutName 布局名称
     * @param dependency 依赖的布局名称
     * @return 是否添加成功
     */
    bool addLayoutDependency(const std::string& layoutName, const std::string& dependency);

    /**
     * @brief 移除布局依赖
     * @param layoutName 布局名称
     * @param dependency 依赖的布局名称
     * @return 是否移除成功
     */
    bool removeLayoutDependency(const std::string& layoutName, const std::string& dependency);

    // === 布局状态管理 ===

    /**
     * @brief 设置布局状态
     * @param layoutName 布局名称
     * @param state 布局状态
     * @return 是否设置成功
     */
    bool setLayoutState(const std::string& layoutName, LayoutState state);

    /**
     * @brief 获取布局状态
     * @param layoutName 布局名称
     * @return 布局状态
     */
    LayoutState getLayoutState(const std::string& layoutName) const;

    /**
     * @brief 获取处于特定状态的布局列表
     * @param state 布局状态
     * @return 布局名称列表
     */
    std::vector<std::string> getLayoutsByState(LayoutState state) const;

    // === 布局元数据管理 ===

    /**
     * @brief 设置布局自定义数据
     * @param layoutName 布局名称
     * @param key 数据键
     * @param value 数据值
     * @return 是否设置成功
     */
    bool setLayoutMetadata(const std::string& layoutName, const std::string& key, const std::string& value);

    /**
     * @brief 获取布局自定义数据
     * @param layoutName 布局名称
     * @param key 数据键
     * @return 数据值，如果不存在则返回空字符串
     */
    std::string getLayoutMetadata(const std::string& layoutName, const std::string& key) const;

    /**
     * @brief 标记布局为需要保存
     * @param layoutName 布局名称
     * @param dirty 是否需要保存
     */
    void markLayoutDirty(const std::string& layoutName, bool dirty = true);

    /**
     * @brief 检查布局是否需要保存
     * @param layoutName 布局名称
     * @return 是否需要保存
     */
    bool isLayoutDirty(const std::string& layoutName) const;

    // === 布局生命周期管理 ===

    /**
     * @brief 激活布局
     * @param layoutName 布局名称
     * @return 是否激活成功
     */
    bool activateLayout(const std::string& layoutName);

    /**
     * @brief 停用布局
     * @param layoutName 布局名称
     * @return 是否停用成功
     */
    bool deactivateLayout(const std::string& layoutName);

    /**
     * @brief 获取最后激活的布局
     * @return 布局名称，如果没有则返回空字符串
     */
    std::string getLastActiveLayout() const;

    /**
     * @brief 自动解决布局冲突
     * @param layoutName 要激活的布局名称
     * @param windowId 窗口ID（可选）
     * @return 是否解决成功
     */
    bool resolveLayoutConflicts(const std::string& layoutName, const std::string& windowId = "");

    // === 布局间通信机制 ===

    /**
     * @brief 发送布局消息
     * @param fromWindowId 源窗口ID
     * @param fromLayoutName 源布局名称
     * @param toWindowId 目标窗口ID（为空则广播到所有窗口）
     * @param toLayoutName 目标布局名称（为空则发送到指定窗口的所有布局）
     * @param message 消息内容
     * @return 是否发送成功
     */
    bool sendLayoutMessage(const std::string& fromWindowId, const std::string& fromLayoutName,
                          const std::string& toWindowId, const std::string& toLayoutName,
                          const std::string& message);

    /**
     * @brief 注册布局消息处理器
     * @param windowId 窗口ID
     * @param layoutName 布局名称
     * @param handler 消息处理函数
     */
    void registerLayoutMessageHandler(const std::string& windowId, const std::string& layoutName,
                                     std::function<void(const std::string& fromWindowId,
                                                       const std::string& fromLayoutName,
                                                       const std::string& message)> handler);

    /**
     * @brief 广播消息到所有窗口
     * @param fromWindowId 源窗口ID
     * @param fromLayoutName 源布局名称
     * @param message 消息内容
     */
    void broadcastMessage(const std::string& fromWindowId, const std::string& fromLayoutName,
                         const std::string& message);

    /**
     * @brief 获取所有已注册的窗口ID
     * @return 窗口ID列表
     */
    std::vector<std::string> getRegisteredWindowIds() const;

    /**
     * @brief 设置当前活跃窗口
     * @param windowId 窗口ID
     */
    void setActiveWindow(const std::string& windowId);

private:
    /**
     * @brief 禁止拷贝构造
     */
    LayoutManager(const LayoutManager&) = delete;
    
    /**
     * @brief 禁止赋值操作
     */
    LayoutManager& operator=(const LayoutManager&) = delete;
  
    // 多窗口布局存储：windowId -> (layoutName -> layout)
    std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<LayoutBase>>> windowLayouts_; ///< 窗口布局映射

    // 窗口上下文管理
    std::unordered_map<std::string, WindowBase*> windowContexts_; ///< 窗口上下文映射
    std::string defaultWindowId_;                                  ///< 默认窗口ID

    // 布局消息处理器
    std::unordered_map<std::string, std::unordered_map<std::string,
        std::function<void(const std::string&, const std::string&, const std::string&)>>> messageHandlers_; ///< 布局消息处理器映射

    // 事件系统相关（全局共享）
    Events::LayoutEventDispatcher* eventDispatcher_;                        ///< 布局事件调度器

    // 布局注册机制相关（全局共享）
    std::unordered_map<std::string, LayoutRegistration> registeredLayouts_; ///< 已注册的布局类型
    std::unordered_map<std::string, LayoutMetadata> layoutMetadata_;        ///< 布局元数据

    // 布局管理相关（按窗口存储）
    std::unordered_map<std::string, std::string> lastActiveLayouts_;        ///< 每个窗口最后激活的布局名称
    std::unordered_map<std::string, std::string> currentContentLayouts_;    ///< 每个窗口当前可见的内容布局名称
    std::unordered_map<std::string, std::vector<std::string>> systemLayoutNames_; ///< 每个窗口的系统布局名称列表
    std::chrono::steady_clock::time_point lastUpdateTime_;                  ///< 最后更新时间
    std::string currentWindowId_;                                          ///< 当前活跃窗口ID

    // 辅助方法
    std::string getCurrentWindowId() const;                                ///< 获取当前窗口ID
    std::string getLayoutWindowId(const std::string& layoutName) const;    ///< 获取布局所属窗口ID

}; // class LayoutManager

} // namespace Window
} // namespace Core
} // namespace DearTs