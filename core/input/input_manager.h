/**
 * DearTs Input System Header - Simplified Implementation
 * 
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#pragma once

// Logger removed - using simple output instead
#include <SDL.h>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>

namespace DearTs {
namespace Core {
namespace Input {

/**
 * @brief 键盘按键代码
 */
enum class KeyCode {
    // 字母键
    A = SDL_SCANCODE_A, B = SDL_SCANCODE_B, C = SDL_SCANCODE_C, D = SDL_SCANCODE_D,
    E = SDL_SCANCODE_E, F = SDL_SCANCODE_F, G = SDL_SCANCODE_G, H = SDL_SCANCODE_H,
    I = SDL_SCANCODE_I, J = SDL_SCANCODE_J, K = SDL_SCANCODE_K, L = SDL_SCANCODE_L,
    M = SDL_SCANCODE_M, N = SDL_SCANCODE_N, O = SDL_SCANCODE_O, P = SDL_SCANCODE_P,
    Q = SDL_SCANCODE_Q, R = SDL_SCANCODE_R, S = SDL_SCANCODE_S, T = SDL_SCANCODE_T,
    U = SDL_SCANCODE_U, V = SDL_SCANCODE_V, W = SDL_SCANCODE_W, X = SDL_SCANCODE_X,
    Y = SDL_SCANCODE_Y, Z = SDL_SCANCODE_Z,
    
    // 数字键
    NUM_0 = SDL_SCANCODE_0, NUM_1 = SDL_SCANCODE_1, NUM_2 = SDL_SCANCODE_2,
    NUM_3 = SDL_SCANCODE_3, NUM_4 = SDL_SCANCODE_4, NUM_5 = SDL_SCANCODE_5,
    NUM_6 = SDL_SCANCODE_6, NUM_7 = SDL_SCANCODE_7, NUM_8 = SDL_SCANCODE_8,
    NUM_9 = SDL_SCANCODE_9,
    
    // 方向键
    UP = SDL_SCANCODE_UP, DOWN = SDL_SCANCODE_DOWN,
    LEFT = SDL_SCANCODE_LEFT, RIGHT = SDL_SCANCODE_RIGHT,
    
    // 特殊键
    SPACE = SDL_SCANCODE_SPACE, ENTER = SDL_SCANCODE_RETURN,
    ESCAPE = SDL_SCANCODE_ESCAPE, BACKSPACE = SDL_SCANCODE_BACKSPACE,
    TAB = SDL_SCANCODE_TAB,
    
    UNKNOWN = SDL_SCANCODE_UNKNOWN
};

/**
 * @brief 鼠标按键代码
 */
enum class MouseButton {
    LEFT = SDL_BUTTON_LEFT,
    MIDDLE = SDL_BUTTON_MIDDLE,
    RIGHT = SDL_BUTTON_RIGHT,
    UNKNOWN = 0
};

/**
 * @brief 输入状态
 */
enum class InputState {
    RELEASED,
    PRESSED,
    HELD
};

/**
 * @brief 2D向量结构
 */
struct Vector2 {
    float x, y;
    
    Vector2() : x(0.0f), y(0.0f) {}
    Vector2(float x_, float y_) : x(x_), y(y_) {}
};

/**
 * @brief 输入管理器类 - 简化版本
 */
class InputManager {
public:
    /**
     * @brief 获取单例实例
     */
    static InputManager& getInstance();
    
    /**
     * @brief 初始化输入管理器
     */
    bool initialize();
    
    /**
     * @brief 关闭输入管理器
     */
    void shutdown();
    
    /**
     * @brief 更新输入状态
     */
    void update();
    
    /**
     * @brief 处理SDL事件
     */
    bool handleEvent(const SDL_Event& event);
    
    // 键盘输入查询
    bool isKeyPressed(KeyCode key) const;
    bool isKeyHeld(KeyCode key) const;
    bool wasKeyJustPressed(KeyCode key) const;
    bool wasKeyJustReleased(KeyCode key) const;
    
    // 鼠标输入查询
    bool isMouseButtonPressed(MouseButton button) const;
    bool isMouseButtonHeld(MouseButton button) const;
    bool wasMouseButtonJustPressed(MouseButton button) const;
    bool wasMouseButtonJustReleased(MouseButton button) const;
    Vector2 getMousePosition() const;
    Vector2 getMouseDelta() const;
    
private:
    InputManager() = default;
    ~InputManager() = default;
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;
    
    bool initialized_ = false;
    
    // 键盘状态
    std::unordered_map<KeyCode, InputState> key_states_;
    std::unordered_map<KeyCode, InputState> previous_key_states_;
    
    // 鼠标状态
    std::unordered_map<MouseButton, InputState> button_states_;
    std::unordered_map<MouseButton, InputState> previous_button_states_;
    Vector2 mouse_position_;
    Vector2 mouse_delta_;
    
    mutable std::mutex mutex_;
    
    /**
     * @brief 更新键盘状态
     */
    void updateKeyState(KeyCode key, bool pressed);
    
    /**
     * @brief 更新鼠标按键状态
     */
    void updateButtonState(MouseButton button, bool pressed);
};

#define DEARTS_INPUT InputManager::getInstance()
#define DEARTS_KEY_PRESSED(key) DEARTS_INPUT.isKeyPressed(key)
#define DEARTS_KEY_JUST_PRESSED(key) DEARTS_INPUT.wasKeyJustPressed(key)
#define DEARTS_MOUSE_PRESSED(button) DEARTS_INPUT.isMouseButtonPressed(button)
#define DEARTS_MOUSE_JUST_PRESSED(button) DEARTS_INPUT.wasMouseButtonJustPressed(button)
#define DEARTS_MOUSE_POS() DEARTS_INPUT.getMousePosition()

} // namespace Input
} // namespace Core
} // namespace DearTs
