/**
 * @file tools.cpp
 * @brief 工具注册表实现
 */

#include "core/content/tools.h"
#include "liblogger/logger.h"

namespace DearTs::Core::ContentRegistry::Tools {

ToolItem& Registry::add(const UnlocalizedString& name,
                       const std::string& description,
                       Callback callback) {
    m_tools.push_back(ToolItem{
        .name = name,
        .description = description,
        .callback = std::move(callback)
    });

    LOG_INFO("Added tool: {} - {}", name.get(), description);
    return m_tools.back();
}

const std::vector<ToolItem>& Registry::get_all() const {
    return m_tools;
}

void Registry::clear() {
    m_tools.clear();
}

} // namespace DearTs::Core::ContentRegistry::Tools
