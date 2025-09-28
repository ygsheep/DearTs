/**
 * DearTs Input System Implementation - Simplified Implementation
 * 
 * 简化的输入系统实现文件
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#include "input_manager.h"
#include "../core.h"
// Logger removed - using simple output instead
#include <algorithm>

namespace DearTs {
namespace Core {
namespace Input {

// ============================================================================
// InputManager 实现
// ============================================================================

/**
 * @brief 获取单例实例
 */
InputManager& InputManager::getInstance() {
    static InputManager instance;
    return instance;
}

/**
 * @brief 初始化输入管理器
 */
bool InputManager::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        DEARTS_LOG_WARN("输入管理器已初始化");
        return true;
    }
    
    // 清空状态
    key_states_.clear();
    previous_key_states_.clear();
    button_states_.clear();
    previous_button_states_.clear();
    
    mouse_position_ = Vector2(0.0f, 0.0f);
    mouse_delta_ = Vector2(0.0f, 0.0f);
    
    initialized_ = true;
    DEARTS_LOG_INFO("输入管理器初始化成功");
    return true;
}

/**
 * @brief 关闭输入管理器
 */
void InputManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return;
    }
    
    // 清空所有状态
    key_states_.clear();
    previous_key_states_.clear();
    button_states_.clear();
    previous_button_states_.clear();
    
    initialized_ = false;
    DEARTS_LOG_INFO("输入管理器关闭");
}

/**
 * @brief 更新输入状态
 */
void InputManager::update() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return;
    }
    
    // 更新之前的状态
    previous_key_states_ = key_states_;
    previous_button_states_ = button_states_;
    
    // 更新按键状态
    for (auto& [key, state] : key_states_) {
        if (state == InputState::PRESSED) {
            state = InputState::HELD;
        }
    }
    
    // 更新鼠标按键状态
    for (auto& [button, state] : button_states_) {
        if (state == InputState::PRESSED) {
            state = InputState::HELD;
        }
    }
    
    // 重置鼠标增量
    mouse_delta_ = Vector2(0.0f, 0.0f);
}

/**
 * @brief 处理SDL事件
 */
bool InputManager::handleEvent(const SDL_Event& event) {
    if (!initialized_) {
        return false;
    }
    
    switch (event.type) {
        case SDL_KEYDOWN:
            updateKeyState(static_cast<KeyCode>(event.key.keysym.scancode), true);
            return true;
            
        case SDL_KEYUP:
            updateKeyState(static_cast<KeyCode>(event.key.keysym.scancode), false);
            return true;
            
        case SDL_MOUSEBUTTONDOWN:
            updateButtonState(static_cast<MouseButton>(event.button.button), true);
            return true;
            
        case SDL_MOUSEBUTTONUP:
            updateButtonState(static_cast<MouseButton>(event.button.button), false);
            return true;
            
        case SDL_MOUSEMOTION: {
            std::lock_guard<std::mutex> lock(mutex_);
            Vector2 new_position(static_cast<float>(event.motion.x), static_cast<float>(event.motion.y));
            mouse_delta_ = Vector2(static_cast<float>(event.motion.xrel), static_cast<float>(event.motion.yrel));
            mouse_position_ = new_position;
            DEARTS_LOG_DEBUG("Mouse moved to (" + std::to_string(event.motion.x) + ", " + std::to_string(event.motion.y) + ")");
            return true;
        }
        
        default:
            return false;
    }
}

// ============================================================================
// 键盘输入查询
// ============================================================================

/**
 * @brief 检查按键是否被按下
 */
bool InputManager::isKeyPressed(KeyCode key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = key_states_.find(key);
    return it != key_states_.end() && it->second == InputState::PRESSED;
}

/**
 * @brief 检查按键是否持续按住
 */
bool InputManager::isKeyHeld(KeyCode key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = key_states_.find(key);
    return it != key_states_.end() && (it->second == InputState::PRESSED || it->second == InputState::HELD);
}

/**
 * @brief 检查按键是否刚刚被按下
 */
bool InputManager::wasKeyJustPressed(KeyCode key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto current_it = key_states_.find(key);
    auto previous_it = previous_key_states_.find(key);
    
    bool current_pressed = (current_it != key_states_.end() && current_it->second == InputState::PRESSED);
    bool previous_pressed = (previous_it != previous_key_states_.end() && 
                           (previous_it->second == InputState::PRESSED || previous_it->second == InputState::HELD));
    
    return current_pressed && !previous_pressed;
}

/**
 * @brief 检查按键是否刚刚被释放
 */
bool InputManager::wasKeyJustReleased(KeyCode key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto current_it = key_states_.find(key);
    auto previous_it = previous_key_states_.find(key);
    
    bool current_pressed = (current_it != key_states_.end() && 
                          (current_it->second == InputState::PRESSED || current_it->second == InputState::HELD));
    bool previous_pressed = (previous_it != previous_key_states_.end() && 
                           (previous_it->second == InputState::PRESSED || previous_it->second == InputState::HELD));
    
    return !current_pressed && previous_pressed;
}

// ============================================================================
// 鼠标输入查询
// ============================================================================

/**
 * @brief 检查鼠标按键是否被按下
 */
bool InputManager::isMouseButtonPressed(MouseButton button) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = button_states_.find(button);
    return it != button_states_.end() && it->second == InputState::PRESSED;
}

/**
 * @brief 检查鼠标按键是否持续按住
 */
bool InputManager::isMouseButtonHeld(MouseButton button) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = button_states_.find(button);
    return it != button_states_.end() && (it->second == InputState::PRESSED || it->second == InputState::HELD);
}

/**
 * @brief 检查鼠标按键是否刚刚被按下
 */
bool InputManager::wasMouseButtonJustPressed(MouseButton button) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto current_it = button_states_.find(button);
    auto previous_it = previous_button_states_.find(button);
    
    bool current_pressed = (current_it != button_states_.end() && current_it->second == InputState::PRESSED);
    bool previous_pressed = (previous_it != previous_button_states_.end() && 
                           (previous_it->second == InputState::PRESSED || previous_it->second == InputState::HELD));
    
    return current_pressed && !previous_pressed;
}

/**
 * @brief 检查鼠标按键是否刚刚被释放
 */
bool InputManager::wasMouseButtonJustReleased(MouseButton button) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto current_it = button_states_.find(button);
    auto previous_it = previous_button_states_.find(button);
    
    bool current_pressed = (current_it != button_states_.end() && 
                          (current_it->second == InputState::PRESSED || current_it->second == InputState::HELD));
    bool previous_pressed = (previous_it != previous_button_states_.end() && 
                           (previous_it->second == InputState::PRESSED || previous_it->second == InputState::HELD));
    
    return !current_pressed && previous_pressed;
}

/**
 * @brief 获取鼠标位置
 */
Vector2 InputManager::getMousePosition() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return mouse_position_;
}

/**
 * @brief 获取鼠标移动增量
 */
Vector2 InputManager::getMouseDelta() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return mouse_delta_;
}

// ============================================================================
// 内部方法
// ============================================================================

/**
 * @brief 更新键盘状态
 */
void InputManager::updateKeyState(KeyCode key, bool pressed) {
    if (pressed) {
        key_states_[key] = InputState::PRESSED;
        DEARTS_LOG_DEBUG("Key pressed: " + std::to_string(static_cast<int>(key)) + " (" + std::to_string(static_cast<int>(key)) + ")");
    } else {
        key_states_[key] = InputState::RELEASED;
        DEARTS_LOG_DEBUG("Key released: " + std::to_string(static_cast<int>(key)) + " (" + std::to_string(static_cast<int>(key)) + ")");
    }
}

/**
 * @brief 更新鼠标按键状态
 */
void InputManager::updateButtonState(MouseButton button, bool pressed) {
    if (pressed) {
        button_states_[button] = InputState::PRESSED;
        DEARTS_LOG_DEBUG("Mouse button pressed: " + std::to_string(static_cast<int>(button)));
    } else {
        button_states_[button] = InputState::RELEASED;
        DEARTS_LOG_DEBUG("Mouse button released: " + std::to_string(static_cast<int>(button)));
    }
}

} // namespace Input
} // namespace Core
} // namespace DearTs