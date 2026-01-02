/**
 * @file callbacks.cpp
 * @brief 回调注册表实现
 */

#include "core/content/callbacks.h"
#include "liblogger/logger.h"

namespace DearTs::Core::ContentRegistry::Callbacks {

void Registry::add_on_init(Callback callback) {
    m_init_callbacks.push_back(std::move(callback));
}

void Registry::add_on_shutdown(Callback callback) {
    m_shutdown_callbacks.push_back(std::move(callback));
}

void Registry::add_on_update(std::function<void(double)> callback) {
    m_update_callbacks.push_back(std::move(callback));
}

void Registry::add_on_render(Callback callback) {
    m_render_callbacks.push_back(std::move(callback));
}

void Registry::run_init_callbacks() {
    LOG_INFO("Running {} init callbacks", m_init_callbacks.size());
    for (const auto& callback : m_init_callbacks) {
        try {
            callback();
        } catch (const std::exception& e) {
            LOG_ERROR("Init callback failed: {}", e.what());
        }
    }
}

void Registry::run_shutdown_callbacks() {
    LOG_INFO("Running {} shutdown callbacks", m_shutdown_callbacks.size());
    for (const auto& callback : m_shutdown_callbacks) {
        try {
            callback();
        } catch (const std::exception& e) {
            LOG_ERROR("Shutdown callback failed: {}", e.what());
        }
    }
}

void Registry::run_update_callbacks(double delta_time) {
    for (const auto& callback : m_update_callbacks) {
        try {
            callback(delta_time);
        } catch (const std::exception& e) {
            LOG_ERROR("Update callback failed: {}", e.what());
        }
    }
}

void Registry::run_render_callbacks() {
    for (const auto& callback : m_render_callbacks) {
        try {
            callback();
        } catch (const std::exception& e) {
            LOG_ERROR("Render callback failed: {}", e.what());
        }
    }
}

void Registry::clear() {
    m_init_callbacks.clear();
    m_shutdown_callbacks.clear();
    m_update_callbacks.clear();
    m_render_callbacks.clear();
}

} // namespace DearTs::Core::ContentRegistry::Callbacks
