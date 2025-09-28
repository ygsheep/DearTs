#include <dearts/api/dearts_api.hpp>
#include <SDL.h>
#include <imgui.h>
#include <iostream>
#include <map>
#include <string>

namespace dearts {
    namespace DearTsApi {
        
        namespace Window {
            static SDL_Window* s_mainWindow = nullptr;
            
            SDL_Window* getMainWindow() {
                return s_mainWindow;
            }
            
            void setMainWindow(SDL_Window* window) {
                s_mainWindow = window;
            }
            
            ImVec2 getWindowPosition() {
                if (s_mainWindow) {
                    int x, y;
                    SDL_GetWindowPosition(s_mainWindow, &x, &y);
                    return ImVec2(static_cast<float>(x), static_cast<float>(y));
                }
                return ImVec2(0, 0);
            }
            
            void setWindowPosition(i32 x, i32 y) {
                if (s_mainWindow) {
                    SDL_SetWindowPosition(s_mainWindow, x, y);
                }
            }
            
            ImVec2 getWindowSize() {
                if (s_mainWindow) {
                    int w, h;
                    SDL_GetWindowSize(s_mainWindow, &w, &h);
                    return ImVec2(static_cast<float>(w), static_cast<float>(h));
                }
                return ImVec2(800, 600);
            }
            
            void setWindowSize(u32 width, u32 height) {
                if (s_mainWindow) {
                    SDL_SetWindowSize(s_mainWindow, static_cast<int>(width), static_cast<int>(height));
                }
            }
            
            bool isResizable() {
                if (s_mainWindow) {
                    Uint32 flags = SDL_GetWindowFlags(s_mainWindow);
                    return (flags & SDL_WINDOW_RESIZABLE) != 0;
                }
                return false;
            }
            
            void setResizable(bool resizable) {
                if (s_mainWindow) {
                    SDL_SetWindowResizable(s_mainWindow, resizable ? SDL_TRUE : SDL_FALSE);
                }
            }
        }
        
        namespace System {
            static float s_globalScale = 1.0f;
            
            void closeApplication(bool noQuestions) {
                // 实现应用程序关闭逻辑
                std::cout << "Closing application..." << std::endl;
                exit(0);
            }
            
            void restartApplication() {
                // 实现应用程序重启逻辑
                std::cout << "Restarting application..." << std::endl;
            }
            
            float getGlobalScale() {
                return s_globalScale;
            }
            
            void setGlobalScale(float scale) {
                s_globalScale = scale;
            }
            
            std::string getOSName() {
                #ifdef _WIN32
                return "Windows";
                #elif defined(__linux__)
                return "Linux";
                #elif defined(__APPLE__)
                return "macOS";
                #else
                return "Unknown";
                #endif
            }
            
            std::string getOSVersion() {
                return "Unknown";
            }
            
            std::string getArchitecture() {
                #ifdef _WIN64
                return "x64";
                #elif defined(_WIN32)
                return "x86";
                #else
                return "Unknown";
                #endif
            }
            
            void addStartupTask(const std::string &name, bool async, const std::function<bool()> &function) {
                // 实现启动任务添加逻辑
                std::cout << "Adding startup task: " << name << std::endl;
                if (function) {
                    function();
                }
            }
        }
        
        namespace Theme {
            static std::string s_currentTheme = "Dark";
            static std::map<std::string, ThemeInfo> s_availableThemes = {
                {"Dark", {"Dark", "DearTs Team", "Dark theme for DearTs", "1.0"}},
                {"Light", {"Light", "DearTs Team", "Light theme for DearTs", "1.0"}}
            };
            static bool s_systemThemeDetection = false;
            
            std::string getCurrentTheme() {
                return s_currentTheme;
            }
            
            /**
             * 设置当前主题
             * @param themeName 主题名称
             */
            void setCurrentTheme(const std::string &themeName) {
                if (s_availableThemes.find(themeName) != s_availableThemes.end()) {
                    s_currentTheme = themeName;
                    std::cout << "Theme set to: " << themeName << std::endl;
                } else {
                    std::cout << "Warning: Theme '" << themeName << "' not found, using default." << std::endl;
                }
            }
            
            std::vector<ThemeInfo> getAvailableThemes() {
                std::vector<ThemeInfo> themes;
                for (const auto& pair : s_availableThemes) {
                    themes.push_back(pair.second);
                }
                return themes;
            }
            
            void enableSystemThemeDetection(bool enabled) {
                s_systemThemeDetection = enabled;
            }
            
            bool usesSystemThemeDetection() {
                return s_systemThemeDetection;
            }
        }
        
        namespace Fonts {
            static std::map<std::string, ImFont*> s_fonts;
            
            GlyphRange glyph(const char *glyph) {
                // 简单实现
                return {0, 255};
            }
            
            GlyphRange glyph(u32 codepoint) {
                return {static_cast<u16>(codepoint), static_cast<u16>(codepoint)};
            }
            
            GlyphRange range(const char *glyphBegin, const char *glyphEnd) {
                return {0, 255};
            }
            
            GlyphRange range(u32 codepointBegin, u32 codepointEnd) {
                return {static_cast<u16>(codepointBegin), static_cast<u16>(codepointEnd)};
            }
            
            void loadFont(const std::string &path, const std::vector<GlyphRange> &glyphRanges, 
                         Offset offset, u32 flags, std::optional<u32> defaultSize) {
                std::cout << "Loading font from path: " << path << std::endl;
            }
            
            void loadFont(const std::string &name, const std::span<const u8> &data, 
                         const std::vector<GlyphRange> &glyphRanges, Offset offset, 
                         u32 flags, std::optional<u32> defaultSize) {
                std::cout << "Loading font: " << name << std::endl;
            }
            
            void registerFont(const UnlocalizedString &fontName) {
                std::cout << "Registering font: " << fontName.get() << std::endl;
            }
            
            ImFont* getFont(const UnlocalizedString &fontName) {
                auto it = s_fonts.find(fontName.get());
                return (it != s_fonts.end()) ? it->second : nullptr;
            }
        }
        
        namespace Messaging {
            static std::map<std::string, MessagingHandler> s_handlers;
            
            void registerHandler(const std::string &eventName, const MessagingHandler &handler) {
                s_handlers[eventName] = handler;
                std::cout << "Registered messaging handler for: " << eventName << std::endl;
            }
            
            void sendMessage(const std::string &eventName, const std::vector<u8> &data) {
                auto it = s_handlers.find(eventName);
                if (it != s_handlers.end()) {
                    it->second(data);
                } else {
                    std::cout << "No handler found for event: " << eventName << std::endl;
                }
            }
        }
    }
}