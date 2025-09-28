#pragma once

#include <dearts/dearts.hpp>
#include <dearts/api/event_manager.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <any>
#include <nlohmann/json.hpp>
#include <imgui.h>

using ImGuiID = unsigned int;
struct ImVec2;
struct ImVec4;

namespace dearts {
    
    /**
     * @brief 内容注册表命名空间
     */
    namespace ContentRegistry {
        
        /**
         * @brief 视图管理
         */
        namespace Views {
            
            /**
             * @brief 视图基类
             */
            class View {
            public:
                /**
                 * @brief 构造函数
                 * @param unlocalizedName 未本地化名称
                 */
                explicit View(UnlocalizedString unlocalizedName);
                
                /**
                 * @brief 虚析构函数
                 */
                virtual ~View() = default;
                
                /**
                 * @brief 绘制视图内容
                 */
                virtual void drawContent() = 0;
                
                /**
                 * @brief 绘制菜单项
                 */
                virtual void drawMenu();
                
                /**
                 * @brief 获取未本地化名称
                 * @return 未本地化名称
                 */
                const UnlocalizedString& getUnlocalizedName() const;
                
                /**
                 * @brief 获取显示名称
                 * @return 显示名称
                 */
                std::string getDisplayName() const;
                
                /**
                 * @brief 检查视图是否打开
                 * @return 是否打开
                 */
                bool& getWindowOpenState();
                
                /**
                 * @brief 检查视图是否有菜单项
                 * @return 是否有菜单项
                 */
                bool hasViewMenuItemEntry() const;
                
                /**
                 * @brief 设置是否有菜单项
                 * @param hasEntry 是否有菜单项
                 */
                void setViewMenuItemEntry(bool hasEntry);
                
                /**
                 * @brief 获取ImGui窗口标志
                 * @return 窗口标志
                 */
                virtual ImGuiWindowFlags getWindowFlags() const;
                
            private:
                UnlocalizedString m_unlocalizedViewName;
                bool m_windowOpen = false;
                bool m_hasViewMenuItemEntry = true;
            };
            
            /**
             * @brief 添加视图
             * @tparam T 视图类型
             * @tparam Args 构造参数类型
             * @param args 构造参数
             */
            template<std::derived_from<View> T, typename... Args>
            void add(Args &&...args) {
                return add(std::make_unique<T>(std::forward<Args>(args)...));
            }
            
            /**
             * @brief 添加视图
             * @param view 视图智能指针
             */
            void add(std::unique_ptr<View> view);
            
            /**
             * @brief 获取所有视图
             * @return 视图列表
             */
            std::vector<std::unique_ptr<View>>& getEntries();
            
        }
        
        /**
         * @brief 工具管理
         */
        namespace Tools {
            
            /**
             * @brief 工具条目结构
             */
            struct Entry {
                UnlocalizedString name;                    ///< 工具名称
                std::function<void()> function;           ///< 工具函数
                bool detached = false;                    ///< 是否分离
            };
            
            /**
             * @brief 添加工具
             * @param unlocalizedName 未本地化名称
             * @param function 工具函数
             */
            void add(const UnlocalizedString &unlocalizedName, const std::function<void()> &function);
            
            /**
             * @brief 获取所有工具条目
             * @return 工具条目列表
             */
            std::vector<Entry>& getEntries();
            
        }
        
        /**
         * @brief 数据检查器管理
         */
        namespace DataInspector {
            
            /**
             * @brief 数据检查器条目结构
             */
            struct Entry {
                UnlocalizedString unlocalizedName;                           ///< 未本地化名称
                size_t requiredSize;                                        ///< 所需大小
                size_t maxSize;                                             ///< 最大大小
                std::function<std::string(std::span<const u8>)> displayFunction; ///< 显示函数
                std::optional<std::function<std::string(std::string)>> editingFunction; ///< 编辑函数
            };
            
            /**
             * @brief 添加数据检查器条目
             * @param unlocalizedName 未本地化名称
             * @param requiredSize 所需大小
             * @param displayFunction 显示函数
             * @param editingFunction 编辑函数
             */
            void add(const UnlocalizedString &unlocalizedName, size_t requiredSize,
                    const std::function<std::string(std::span<const u8>)> &displayFunction,
                    const std::optional<std::function<std::string(std::string)>> &editingFunction = std::nullopt);
            
            /**
             * @brief 添加数据检查器条目（可变大小）
             * @param unlocalizedName 未本地化名称
             * @param requiredSize 所需大小
             * @param maxSize 最大大小
             * @param displayFunction 显示函数
             * @param editingFunction 编辑函数
             */
            void add(const UnlocalizedString &unlocalizedName, size_t requiredSize, size_t maxSize,
                    const std::function<std::string(std::span<const u8>)> &displayFunction,
                    const std::optional<std::function<std::string(std::string)>> &editingFunction = std::nullopt);
            
            /**
             * @brief 获取所有数据检查器条目
             * @return 数据检查器条目列表
             */
            std::vector<Entry>& getEntries();
            
        }
        
        /**
         * @brief 语言管理
         */
        namespace Language {
            
            /**
             * @brief 本地化条目结构
             */
            struct LocalizationEntry {
                std::string key;                          ///< 键
                std::string value;                        ///< 值
            };
            
            /**
             * @brief 添加本地化条目
             * @param language 语言代码
             * @param key 键
             * @param value 值
             */
            void addLocalization(const std::string &language, const std::string &key, const std::string &value);
            
            /**
             * @brief 获取本地化字符串
             * @param key 键
             * @return 本地化字符串
             */
            std::string getLocalizedString(const std::string &key);
            
            /**
             * @brief 设置当前语言
             * @param language 语言代码
             */
            void setCurrentLanguage(const std::string &language);
            
            /**
             * @brief 获取当前语言
             * @return 当前语言代码
             */
            std::string getCurrentLanguage();
            
            /**
             * @brief 获取可用语言列表
             * @return 语言代码列表
             */
            std::vector<std::string> getAvailableLanguages();
            
        }
        
        /**
         * @brief 接口管理
         */
        namespace Interface {
            
            /**
             * @brief 绘制函数类型
             */
            using DrawCallback = std::function<void()>;
            
            /**
             * @brief 菜单项条目结构
             */
            struct MenuEntry {
                UnlocalizedString unlocalizedName;        ///< 未本地化名称
                std::vector<std::string> path;           ///< 菜单路径
                u32 priority;                            ///< 优先级
                DrawCallback callback;                   ///< 回调函数
                std::function<bool()> enabledCallback;   ///< 启用状态回调
            };
            
            /**
             * @brief 添加菜单项
             * @param unlocalizedName 未本地化名称
             * @param priority 优先级
             * @param callback 回调函数
             * @param enabledCallback 启用状态回调
             */
            void addMenuItem(const UnlocalizedString &unlocalizedName, u32 priority,
                           const DrawCallback &callback, const std::function<bool()> &enabledCallback = []{ return true; });
            
            /**
             * @brief 添加菜单项到指定路径
             * @param path 菜单路径
             * @param priority 优先级
             * @param callback 回调函数
             * @param enabledCallback 启用状态回调
             */
            void addMenuItemToPath(const std::vector<std::string> &path, u32 priority,
                                 const DrawCallback &callback, const std::function<bool()> &enabledCallback = []{ return true; });
            
            /**
             * @brief 添加主菜单项
             * @param unlocalizedName 未本地化名称
             * @param priority 优先级
             * @param callback 回调函数
             */
            void addMainMenuEntry(const UnlocalizedString &unlocalizedName, u32 priority, const DrawCallback &callback);
            
            /**
             * @brief 添加侧边栏项
             * @param icon 图标
             * @param callback 回调函数
             * @param enabledCallback 启用状态回调
             */
            void addSidebarItem(const std::string &icon, const DrawCallback &callback, const std::function<bool()> &enabledCallback = []{ return true; });
            
            /**
             * @brief 添加标题栏按钮
             * @param icon 图标
             * @param unlocalizedTooltip 未本地化工具提示
             * @param callback 回调函数
             */
            void addTitleBarButton(const std::string &icon, const UnlocalizedString &unlocalizedTooltip, const DrawCallback &callback);
            
            /**
             * @brief 获取菜单项列表
             * @return 菜单项列表
             */
            std::multimap<u32, MenuEntry>& getMenuEntries();
            
        }
        
        /**
         * @brief 设置管理
         */
        namespace Settings {
            
            /**
             * @brief 设置条目基类
             */
            class Entry {
            public:
                /**
                 * @brief 构造函数
                 * @param unlocalizedName 未本地化名称
                 * @param unlocalizedCategory 未本地化分类
                 */
                Entry(UnlocalizedString unlocalizedName, UnlocalizedString unlocalizedCategory);
                
                /**
                 * @brief 虚析构函数
                 */
                virtual ~Entry() = default;
                
                /**
                 * @brief 绘制设置项
                 */
                virtual void draw() = 0;
                
                /**
                 * @brief 加载设置
                 * @param json JSON对象
                 */
                virtual void load(const nlohmann::json &json) = 0;
                
                /**
                 * @brief 保存设置
                 * @return JSON对象
                 */
                virtual nlohmann::json store() = 0;
                
                /**
                 * @brief 获取未本地化名称
                 * @return 未本地化名称
                 */
                const UnlocalizedString& getUnlocalizedName() const;
                
                /**
                 * @brief 获取未本地化分类
                 * @return 未本地化分类
                 */
                const UnlocalizedString& getUnlocalizedCategory() const;
                
            private:
                UnlocalizedString m_unlocalizedName;
                UnlocalizedString m_unlocalizedCategory;
            };
            
            /**
             * @brief 添加设置条目
             * @tparam T 设置条目类型
             * @tparam Args 构造参数类型
             * @param args 构造参数
             */
            template<std::derived_from<Entry> T, typename... Args>
            void add(Args &&...args) {
                return add(std::make_unique<T>(std::forward<Args>(args)...));
            }
            
            /**
             * @brief 添加设置条目
             * @param entry 设置条目智能指针
             */
            void add(std::unique_ptr<Entry> entry);
            
            /**
             * @brief 获取所有设置条目
             * @return 设置条目列表
             */
            std::vector<std::unique_ptr<Entry>>& getEntries();
            
        }
        
    }
    
}