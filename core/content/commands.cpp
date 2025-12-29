/**
 * @file commands.cpp
 * @brief 命令注册表实现
 */

#include "core/content/commands.h"
#include "liblogger/logger.h"
#include <algorithm>

namespace DearTs::Core::ContentRegistry::Commands {

Registry& Registry::instance() {
    static Registry instance;
    return instance;
}

CommandItem& Registry::add(const UnlocalizedString& name,
                          const std::string& description,
                          Callback callback) {
    m_commands.push_back(CommandItem{
        .name = name,
        .description = description,
        .shortcut = "",
        .callback = std::move(callback),
        .enabled_callback = []() { return true; }
    });

    LOG_INFO("Added command: {} - {}", name.get(), description);
    return m_commands.back();
}

const std::vector<CommandItem>& Registry::get_all() const {
    return m_commands;
}

bool Registry::execute(const UnlocalizedString& name) {
    auto it = std::find_if(m_commands.begin(), m_commands.end(),
        [&name](const CommandItem& item) {
            return item.name == name;
        });

    if (it != m_commands.end()) {
        if (!it->enabled_callback || it->enabled_callback()) {
            it->callback();
            LOG_INFO("Executed command: {}", name.get());
            return true;
        } else {
            LOG_WARN("Command '{}' is disabled", name.get());
            return false;
        }
    }

    LOG_ERROR("Command '{}' not found", name.get());
    return false;
}

std::vector<CommandItem> Registry::search(const std::string& query) const {
    std::vector<CommandItem> results;

    if (query.empty()) {
        results = m_commands;
        return results;
    }

    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

    for (const auto& cmd : m_commands) {
        std::string lower_name = cmd.name.get();
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

        std::string lower_desc = cmd.description;
        std::transform(lower_desc.begin(), lower_desc.end(), lower_desc.begin(), ::tolower);

        if (lower_name.find(lower_query) != std::string::npos ||
            lower_desc.find(lower_query) != std::string::npos) {
            results.push_back(cmd);
        }
    }

    return results;
}

void Registry::clear() {
    m_commands.clear();
}

} // namespace DearTs::Core::ContentRegistry::Commands
