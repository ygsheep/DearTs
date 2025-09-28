#pragma once

#include <dearts/dearts.hpp>
#include <dearts/api/event_manager.hpp>

#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

using ImGuiID = unsigned int;
struct ImVec2;
struct ImFontAtlas;
struct ImFont;
struct SDL_Window;

namespace dearts {
    
    namespace DearTsApi {
        
        /**
         * @brief 窗口管理相关API
         */
        namespace Window {
            
            /**
             * @brief 获取主窗口句柄
             * @return SDL窗口指针
             */
            SDL_Window* getMainWindow();
            
            /**
             * @brief 设置主窗口句柄
             * @param window SDL窗口指针
             */
            void setMainWindow(SDL_Window* window);
            
            /**
             * @brief 获取窗口位置
             * @return 窗口位置
             */
            ImVec2 getWindowPosition();
            
            /**
             * @brief 设置窗口位置
             * @param x X坐标
             * @param y Y坐标
             */
            void setWindowPosition(i32 x, i32 y);
            
            /**
             * @brief 获取窗口大小
             * @return 窗口大小
             */
            ImVec2 getWindowSize();
            
            /**
             * @brief 设置窗口大小
             * @param width 宽度
             * @param height 高度
             */
            void setWindowSize(u32 width, u32 height);
            
            /**
             * @brief 检查窗口是否可调整大小
             * @return 是否可调整大小
             */
            bool isResizable();
            
            /**
             * @brief 设置窗口是否可调整大小
             * @param resizable 是否可调整大小
             */
            void setResizable(bool resizable);
            
        }
        
        /**
         * @brief 系统相关API
         */
        namespace System {
            
            /**
             * @brief 程序参数结构
             */
            struct ProgramArguments {
                int argc;
                char **argv;
                char **envp;
            };
            
            /**
             * @brief 关闭应用程序
             * @param noQuestions 是否跳过确认对话框
             */
            void closeApplication(bool noQuestions = false);
            
            /**
             * @brief 重启应用程序
             */
            void restartApplication();
            
            /**
             * @brief 获取全局缩放比例
             * @return 缩放比例
             */
            float getGlobalScale();
            
            /**
             * @brief 设置全局缩放比例
             * @param scale 缩放比例
             */
            void setGlobalScale(float scale);
            
            /**
             * @brief 获取操作系统名称
             * @return 操作系统名称
             */
            std::string getOSName();
            
            /**
             * @brief 获取操作系统版本
             * @return 操作系统版本
             */
            std::string getOSVersion();
            
            /**
             * @brief 获取系统架构
             * @return 系统架构
             */
            std::string getArchitecture();
            
            /**
             * @brief 添加启动任务
             * @param name 任务名称
             * @param async 是否异步执行
             * @param function 任务函数
             */
            void addStartupTask(const std::string &name, bool async, const std::function<bool()> &function);
            
        }
        
        /**
         * @brief 主题管理相关API
         */
        namespace Theme {
            
            /**
             * @brief 主题信息结构
             */
            struct ThemeInfo {
                std::string name;
                std::string author;
                std::string description;
                std::string version;
            };
            
            /**
             * @brief 获取当前主题
             * @return 当前主题名称
             */
            std::string getCurrentTheme();
            
            /**
             * @brief 设置当前主题
             * @param themeName 主题名称
             */
            void setCurrentTheme(const std::string &themeName);
            
            /**
             * @brief 获取可用主题列表
             * @return 主题列表
             */
            std::vector<ThemeInfo> getAvailableThemes();
            
            /**
             * @brief 启用系统主题检测
             * @param enabled 是否启用
             */
            void enableSystemThemeDetection(bool enabled);
            
            /**
             * @brief 检查是否使用系统主题检测
             * @return 是否使用系统主题检测
             */
            bool usesSystemThemeDetection();
            
        }
        
        /**
         * @brief 字体管理相关API
         */
        namespace Fonts {
            
            /**
             * @brief 字形范围结构
             */
            struct GlyphRange { 
                u16 begin, end; 
            };
            
            /**
             * @brief 字体偏移结构
             */
            struct Offset { 
                float x, y; 
            };
            
            /**
             * @brief 字体信息结构
             */
            struct Font {
                std::string name;
                std::vector<u8> fontData;
                std::vector<GlyphRange> glyphRanges;
                Offset offset;
                u32 flags;
                std::optional<u32> defaultSize;
            };
            
            /**
             * @brief 创建字形范围
             * @param glyph 字形字符
             * @return 字形范围
             */
            GlyphRange glyph(const char *glyph);
            
            /**
             * @brief 创建字形范围
             * @param codepoint 字符编码
             * @return 字形范围
             */
            GlyphRange glyph(u32 codepoint);
            
            /**
             * @brief 创建字形范围
             * @param glyphBegin 起始字符
             * @param glyphEnd 结束字符
             * @return 字形范围
             */
            GlyphRange range(const char *glyphBegin, const char *glyphEnd);
            
            /**
             * @brief 创建字形范围
             * @param codepointBegin 起始编码
             * @param codepointEnd 结束编码
             * @return 字形范围
             */
            GlyphRange range(u32 codepointBegin, u32 codepointEnd);
            
            /**
             * @brief 加载字体文件
             * @param path 字体文件路径
             * @param glyphRanges 字形范围列表
             * @param offset 字体偏移
             * @param flags 字体标志
             * @param defaultSize 默认大小
             */
            void loadFont(const std::string &path, const std::vector<GlyphRange> &glyphRanges = {}, 
                         Offset offset = {}, u32 flags = 0, std::optional<u32> defaultSize = std::nullopt);
            
            /**
             * @brief 从内存加载字体
             * @param name 字体名称
             * @param data 字体数据
             * @param glyphRanges 字形范围列表
             * @param offset 字体偏移
             * @param flags 字体标志
             * @param defaultSize 默认大小
             */
            void loadFont(const std::string &name, const std::span<const u8> &data, 
                         const std::vector<GlyphRange> &glyphRanges = {}, Offset offset = {}, 
                         u32 flags = 0, std::optional<u32> defaultSize = std::nullopt);
            
            /**
             * @brief 注册字体
             * @param fontName 字体名称
             */
            void registerFont(const UnlocalizedString &fontName);
            
            /**
             * @brief 获取字体
             * @param fontName 字体名称
             * @return ImGui字体指针
             */
            ImFont* getFont(const UnlocalizedString &fontName);
            
            constexpr static float DefaultFontSize = 13.0f;
            
        }
        
        /**
         * @brief 消息传递相关API
         */
        namespace Messaging {
            
            using MessagingHandler = std::function<void(const std::vector<u8> &)>;
            
            /**
             * @brief 注册消息处理器
             * @param eventName 事件名称
             * @param handler 处理器函数
             */
            void registerHandler(const std::string &eventName, const MessagingHandler &handler);
            
            /**
             * @brief 发送消息
             * @param eventName 事件名称
             * @param data 消息数据
             */
            void sendMessage(const std::string &eventName, const std::vector<u8> &data);
            
        }
        
    }
    
}