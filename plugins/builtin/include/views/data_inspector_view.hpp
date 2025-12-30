/**
 * @file data_inspector_view.hpp
 * @brief 数据检查器视图（高级示例）
 * @details 展示一个实用的数据检查和分析视图
 */

#pragma once

#include "core/ui/view.h"
#include "core/ui/icon_font.hpp"
#include <vector>
#include <string>
#include <cstdint>

namespace DearTs::Plugins::Builtin {
using DearTs::Core::ContentRegistry::UnlocalizedString;

/**
 * @brief 数据检查器视图
 *
 * 展示如何创建一个实用的数据检查工具
 */
class DataInspectorView : public Core::UI::ViewWindow {
public:
    explicit DataInspectorView()
        : ViewWindow(UnlocalizedString("数据检查器"), ICON_ANALYTICS) {
    }

    ~DataInspectorView() override = default;

    void draw_content() override {
        // 顶部工具栏
        draw_toolbar();

        ImGui::Separator();

        // 数据类型选择
        draw_data_types();

        ImGui::Separator();

        // 数据输入
        draw_data_input();

        ImGui::Separator();

        // 解析结果显示
        draw_parsed_data();
    }

    ImVec2 get_min_size() const override {
        return ImVec2(500, 400);
    }

private:
    void draw_toolbar() {
        if (ImGui::Button("刷新")) {
            refresh_data();
        }

        ImGui::SameLine();
        if (ImGui::Button("清除")) {
            m_data_input.clear();
            m_parsed_values.clear();
        }

        ImGui::SameLine();
        ImGui::Checkbox("自动解析", &m_auto_parse);
    }

    void draw_data_types() {
        ImGui::Text("数据类型:");

        ImGui::BeginGroup();
        if (ImGui::RadioButton("UInt8", m_selected_type == DataType::UInt8)) {
            m_selected_type = DataType::UInt8;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Int8", m_selected_type == DataType::Int8)) {
            m_selected_type = DataType::Int8;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("UInt16", m_selected_type == DataType::UInt16)) {
            m_selected_type = DataType::UInt16;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Int16", m_selected_type == DataType::Int16)) {
            m_selected_type = DataType::Int16;
        }

        if (ImGui::RadioButton("UInt32", m_selected_type == DataType::UInt32)) {
            m_selected_type = DataType::UInt32;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Int32", m_selected_type == DataType::Int32)) {
            m_selected_type = DataType::Int32;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Float", m_selected_type == DataType::Float)) {
            m_selected_type = DataType::Float;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Double", m_selected_type == DataType::Double)) {
            m_selected_type = DataType::Double;
        }
        ImGui::EndGroup();
    }

    void draw_data_input() {
        ImGui::Text("输入数据 (十六进制):");

        char buffer[256];
        strncpy(buffer, m_data_input.c_str(), sizeof(buffer));

        if (ImGui::InputText("##data_input", buffer, sizeof(buffer),
                             ImGuiInputTextFlags_CharsHexadecimal |
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            m_data_input = buffer;
            if (m_auto_parse) {
                parse_data();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("解析")) {
            parse_data();
        }

        // 显示输入长度
        ImGui::SameLine();
        ImGui::TextDisabled("(字节: %zu)", m_data_input.size() / 2);
    }

    void draw_parsed_data() {
        if (m_parsed_values.empty()) {
            ImGui::TextDisabled("暂无解析数据");
            return;
        }

        ImGui::Text("解析结果:");
        if (ImGui::BeginTable("ParsedData", 3,
                              ImGuiTableFlags_Borders |
                              ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("偏移");
            ImGui::TableSetupColumn("原始数据");
            ImGui::TableSetupColumn("解析值");
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < m_parsed_values.size(); ++i) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("0x%zX", i * get_data_size());

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", m_parsed_values[i].hex.c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0, 0.8f, 1, 1), "%s",
                                  m_parsed_values[i].value.c_str());
            }

            ImGui::EndTable();
        }
    }

    void parse_data() {
        m_parsed_values.clear();

        // 简单的十六进制解析
        std::vector<uint8_t> bytes;
        for (size_t i = 0; i < m_data_input.size(); i += 2) {
            std::string byte_str = m_data_input.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(strtol(byte_str.c_str(), nullptr, 16));
            bytes.push_back(byte);
        }

        // 根据选择的数据类型解析
        size_t data_size = get_data_size();
        for (size_t i = 0; i + data_size <= bytes.size(); i += data_size) {
            ParsedValue pv;
            pv.hex = format_hex(bytes, i, data_size);
            pv.value = parse_value(bytes, i);
            m_parsed_values.push_back(pv);
        }
    }

    std::string parse_value(const std::vector<uint8_t>& bytes, size_t offset) {
        switch (m_selected_type) {
            case DataType::UInt8:
                return std::to_string(bytes[offset]);
            case DataType::Int8:
                return std::to_string(static_cast<int8_t>(bytes[offset]));
            case DataType::UInt16: {
                uint16_t val = bytes[offset] | (bytes[offset + 1] << 8);
                return std::to_string(val);
            }
            case DataType::Int16: {
                int16_t val = bytes[offset] | (bytes[offset + 1] << 8);
                return std::to_string(val);
            }
            case DataType::UInt32: {
                uint32_t val = bytes[offset] | (bytes[offset + 1] << 8) |
                              (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24);
                return std::to_string(val);
            }
            case DataType::Int32: {
                int32_t val = bytes[offset] | (bytes[offset + 1] << 8) |
                             (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24);
                return std::to_string(val);
            }
            case DataType::Float: {
                float val;
                memcpy(&val, &bytes[offset], sizeof(float));
                return std::to_string(val);
            }
            case DataType::Double: {
                double val;
                memcpy(&val, &bytes[offset], sizeof(double));
                return std::to_string(val);
            }
            default:
                return "Unknown";
        }
    }

    std::string format_hex(const std::vector<uint8_t>& bytes, size_t offset, size_t size) {
        std::string result;
        for (size_t i = 0; i < size && offset + i < bytes.size(); ++i) {
            char buf[4];
            snprintf(buf, sizeof(buf), "%02X ", bytes[offset + i]);
            result += buf;
        }
        return result;
    }

    size_t get_data_size() const {
        switch (m_selected_type) {
            case DataType::UInt8:
            case DataType::Int8:
                return 1;
            case DataType::UInt16:
            case DataType::Int16:
                return 2;
            case DataType::UInt32:
            case DataType::Int32:
            case DataType::Float:
                return 4;
            case DataType::Double:
                return 8;
            default:
                return 1;
        }
    }

    void refresh_data() {
        if (!m_data_input.empty() && m_auto_parse) {
            parse_data();
        }
    }

private:
    enum class DataType {
        UInt8,
        Int8,
        UInt16,
        Int16,
        UInt32,
        Int32,
        Float,
        Double
    };

    struct ParsedValue {
        std::string hex;
        std::string value;
    };

    DataType m_selected_type = DataType::UInt32;
    std::string m_data_input;
    std::vector<ParsedValue> m_parsed_values;
    bool m_auto_parse = true;
};

} // namespace DearTs::Plugins::Builtin
