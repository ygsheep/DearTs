/**
 * @file memory_extractor.cpp
 * @brief 记忆提取器实现
 */

#include "memory_core/memory/memory_extractor.hpp"
#include "memory_core/memory/memory_manager.hpp"
#include "liblogger/logger.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace DearTs::Plugins::MemoryCore::Memory {

// ============ 单例实现 ============

MemoryExtractor& MemoryExtractor::instance() {
    static MemoryExtractor instance;
    return instance;
}

// ============ 规则管理 ============

void MemoryExtractor::initialize_default_rules() {
    LOG_INFO("Initializing default extraction rules");

    // 添加内置规则
    add_rule(BuiltInRules::create_preference_rules());
    add_rule(BuiltInRules::create_fact_rules());
    add_rule(BuiltInRules::create_skill_rules());
    add_rule(BuiltInRules::create_context_rules());

    LOG_INFO("Initialized {} extraction rules", m_rules.size());
}

void MemoryExtractor::add_rule(const ExtractionRule& rule) {
    m_rules.push_back(rule);
    LOG_INFO("Added extraction rule: {}", rule.name);
}

void MemoryExtractor::remove_rule(const std::string& name) {
    auto it = std::remove_if(m_rules.begin(), m_rules.end(),
        [&name](const ExtractionRule& rule) { return rule.name == name; });
    m_rules.erase(it, m_rules.end());
    LOG_INFO("Removed extraction rule: {}", name);
}

void MemoryExtractor::set_rule_enabled(const std::string& name, bool enabled) {
    for (auto& rule : m_rules) {
        if (rule.name == name) {
            rule.enabled = enabled;
            LOG_INFO("{} rule: {}", enabled ? "Enabled" : "Disabled", name);
            return;
        }
    }
    LOG_WARN("Rule not found: {}", name);
}

// ============ 记忆提取 ============

DearTs::Core::Result<std::vector<ExtractionResult>, std::string>
MemoryExtractor::extract_from_message(
    const std::string& message,
    const std::string& conversation_id,
    int64_t source_message_id
) {
    LOG_DEBUG("Extracting memories from message: {}", message.substr(0, 100));

    std::vector<ExtractionResult> all_results;

    // 应用所有启用的规则
    for (const auto& rule : m_rules) {
        if (!rule.enabled) {
            continue;
        }

        auto results = apply_rule(message, rule, conversation_id, source_message_id);
        all_results.insert(all_results.end(), results.begin(), results.end());
    }

    LOG_INFO("Extracted {} memories from message", all_results.size());
    return DearTs::Core::Result<std::vector<ExtractionResult>, std::string>::ok(all_results);
}

DearTs::Core::Result<std::vector<ExtractionResult>, std::string>
MemoryExtractor::extract_from_messages(
    const std::vector<std::string>& messages,
    const std::string& conversation_id
) {
    LOG_INFO("Extracting memories from {} messages", messages.size());

    std::vector<ExtractionResult> all_results;

    for (size_t i = 0; i < messages.size(); ++i) {
        auto result = extract_from_message(messages[i], conversation_id, static_cast<int64_t>(i));
        if (result.isOk()) {
            auto& results = result.unwrap();
            all_results.insert(all_results.end(), results.begin(), results.end());
        }
    }

    LOG_INFO("Extracted {} total memories from {} messages", all_results.size(), messages.size());
    return DearTs::Core::Result<std::vector<ExtractionResult>, std::string>::ok(all_results);
}

DearTs::Core::Result<std::vector<int64_t>, std::string>
MemoryExtractor::extract_and_save(
    const std::string& message,
    const std::string& conversation_id,
    int64_t source_message_id
) {
    // 提取记忆
    auto extract_result = extract_from_message(message, conversation_id, source_message_id);
    if (extract_result.isErr()) {
        return DearTs::Core::Result<std::vector<int64_t>, std::string>::err(extract_result.error());
    }

    auto& extractions = extract_result.unwrap();
    std::vector<int64_t> saved_ids;

    // 保存到数据库
    auto& manager = MemoryManager::instance();
    for (const auto& extraction : extractions) {
        auto add_result = manager.add_memory(extraction.memory);
        if (add_result.isOk()) {
            saved_ids.push_back(add_result.unwrap());
        }
    }

    LOG_INFO("Extracted and saved {} memories", saved_ids.size());
    return DearTs::Core::Result<std::vector<int64_t>, std::string>::ok(saved_ids);
}

// ============ 专用提取器 ============

std::vector<ExtractionResult> MemoryExtractor::extract_preferences(const std::string& message) {
    std::vector<ExtractionResult> results;

    // 查找偏好规则
    for (const auto& rule : m_rules) {
        if (rule.enabled && rule.target_type == MemoryType::Preference) {
            auto rule_results = apply_rule(message, rule);
            results.insert(results.end(), rule_results.begin(), rule_results.end());
        }
    }

    return results;
}

std::vector<ExtractionResult> MemoryExtractor::extract_facts(const std::string& message) {
    std::vector<ExtractionResult> results;

    // 查找事实规则
    for (const auto& rule : m_rules) {
        if (rule.enabled && rule.target_type == MemoryType::Fact) {
            auto rule_results = apply_rule(message, rule);
            results.insert(results.end(), rule_results.begin(), rule_results.end());
        }
    }

    return results;
}

std::vector<ExtractionResult> MemoryExtractor::extract_qa_pairs(
    const std::string& user_message,
    const std::string& assistant_message
) {
    std::vector<ExtractionResult> results;

    // 简单的 QA 对提取：将问题和答案组合
    ExtractionResult result;
    result.memory.type = MemoryType::QA;
    result.memory.content = "Q: " + user_message + "\nA: " + assistant_message;
    result.memory.importance = 0.5;
    result.memory.created_at = 0;
    result.memory.accessed_count = 0;
    result.matched_text = result.memory.content;
    result.rule_name = "qa_pair";
    result.confidence = 0.7;

    results.push_back(result);

    return results;
}

// ============ 私有辅助方法 ============

std::vector<ExtractionResult> MemoryExtractor::apply_rule(
    const std::string& text,
    const ExtractionRule& rule,
    const std::string& conversation_id,
    int64_t source_message_id
) {
    std::vector<ExtractionResult> results;

    for (const auto& pattern : rule.patterns) {
        std::sregex_iterator begin(text.begin(), text.end(), pattern);
        std::sregex_iterator end;

        for (std::sregex_iterator it = begin; it != end; ++it) {
            std::smatch match = *it;
            std::string matched_text = match.str();

            // 创建提取结果
            ExtractionResult result;
            result.memory.type = rule.target_type;
            result.memory.content = clean_extracted_text(matched_text);
            result.memory.importance = rule.default_importance;
            result.memory.created_at = 0;
            result.memory.accessed_count = 0;

            if (!conversation_id.empty()) {
                result.memory.source_conversation_id = conversation_id;
            }
            if (source_message_id > 0) {
                result.memory.source_message_id = source_message_id;
            }

            result.matched_text = matched_text;
            result.rule_name = rule.name;
            result.confidence = calculate_confidence(matched_text, pattern);

            results.push_back(result);
        }
    }

    return results;
}

double MemoryExtractor::calculate_confidence(const std::string& matched_text, const std::regex& pattern) {
    // 简单的置信度计算：基于匹配长度
    // 较长的匹配通常更准确
    size_t length = matched_text.length();

    if (length < 10) return 0.3;
    if (length < 30) return 0.5;
    if (length < 60) return 0.7;
    return 0.9;
}

std::string MemoryExtractor::clean_extracted_text(const std::string& text) {
    // 去除首尾空白
    size_t start = text.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }

    size_t end = text.find_last_not_of(" \t\n\r");
    std::string cleaned = text.substr(start, end - start + 1);

    // 去除多余的空白字符
    std::string result;
    bool in_whitespace = false;
    for (char c : cleaned) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!in_whitespace) {
                result += ' ';
                in_whitespace = true;
            }
        } else {
            result += c;
            in_whitespace = false;
        }
    }

    return result;
}

// ============ 内置规则实现 ============

namespace BuiltInRules {

ExtractionRule create_preference_rules() {
    ExtractionRule rule;
    rule.name = "preferences";
    rule.target_type = MemoryType::Preference;
    rule.default_importance = 0.7;
    rule.enabled = true;

    // 用户偏好模式
    // 例如："我喜欢/偏好/习惯..."
    rule.patterns.push_back(std::regex(R"(我(喜欢|偏好|习惯|常用|通常).{5,50})", std::regex_constants::icase));
    rule.patterns.push_back(std::regex(R"((请|记得|务必)(不要|别).{5,50})", std::regex_constants::ECMAScript | std::regex_constants::icase));
    rule.patterns.push_back(std::regex(R"(设置.{1,20}为.{1,30})", std::regex_constants::icase));

    return rule;
}

ExtractionRule create_fact_rules() {
    ExtractionRule rule;
    rule.name = "facts";
    rule.target_type = MemoryType::Fact;
    rule.default_importance = 0.6;
    rule.enabled = true;

    // 事实知识模式
    // 例如："我是..."/"我在...工作"/"我的项目..."
    rule.patterns.push_back(std::regex(R"(我是.{5,50})", std::regex_constants::icase));
    rule.patterns.push_back(std::regex(R"(我在.{1,20}(公司|团队|部门)工作)", std::regex_constants::icase));
    rule.patterns.push_back(std::regex(R"(我的(名字|项目|任务)是.{1,30})", std::regex_constants::icase));
    rule.patterns.push_back(std::regex(R"((从事|开发).{5,40}(工作|项目))", std::regex_constants::icase));

    return rule;
}

ExtractionRule create_skill_rules() {
    ExtractionRule rule;
    rule.name = "skills";
    rule.target_type = MemoryType::Skill;
    rule.default_importance = 0.5;
    rule.enabled = true;

    // 技能信息模式
    // 例如："我会..."/"擅长..."/"熟悉..."
    rule.patterns.push_back(std::regex(R"(我(会|能|擅长|熟悉).{3,30})", std::regex_constants::icase));
    rule.patterns.push_back(std::regex(R"(使用.{2,20}(开发|编程|语言|工具))", std::regex_constants::icase));
    rule.patterns.push_back(std::regex(R"(技术栈.{5,40})", std::regex_constants::icase));

    return rule;
}

ExtractionRule create_context_rules() {
    ExtractionRule rule;
    rule.name = "context";
    rule.target_type = MemoryType::Context;
    rule.default_importance = 0.4;
    rule.enabled = true;

    // 上下文信息模式
    // 例如："正在..."/"当前项目..."/"关于..."
    rule.patterns.push_back(std::regex(R"(正在(做|开发|实现).{5,50})", std::regex_constants::icase));
    rule.patterns.push_back(std::regex(R"(当前(项目|任务|工作).{5,50})", std::regex_constants::icase));
    rule.patterns.push_back(std::regex(R"(关于.{5,50}(问题|需求|功能))", std::regex_constants::icase));

    return rule;
}

} // namespace BuiltInRules

} // namespace DearTs::Plugins::MemoryCore::Memory
