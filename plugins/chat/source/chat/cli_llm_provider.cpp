/**
 * @file cli_llm_provider.cpp
 * @brief CLI LLM 提供商实现
 */

#include "chat/llm/cli_llm_provider.hpp"
#include "liblogger/logger.h"
#include <fmt/format.h>
#include <sstream>
#include <regex>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/wait.h>
#endif

namespace DearTs::Plugins::Chat::LLM {

CLLILLMProvider::CLLILLMProvider(const std::string& command_template)
    : m_command_template(command_template) {
}

bool CLLILLMProvider::is_available() const {
    // 提取命令的第一部分（可执行文件名）
    std::regex re("^\\s*\"?([^\"]+)\"?");
    std::smatch match;
    std::string first_word;

    if (std::regex_search(m_command_template, match, re) && match.size() > 1) {
        first_word = match[1].str();
    } else {
        return false;
    }

    // 检查命令是否存在
#ifdef _WIN32
    // Windows: 使用 where 命令
    std::string command = fmt::format("where {}", first_word);
    HANDLE hStdOutRead, hStdOutWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };

    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
        return false;
    }

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    PROCESS_INFORMATION pi = {0};

    if (!CreateProcessA(
        nullptr,
        const_cast<char*>(command.c_str()),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    )) {
        CloseHandle(hStdOutRead);
        CloseHandle(hStdOutWrite);
        return false;
    }

    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(hStdOutWrite);

    char buffer[256];
    DWORD bytes_read;
    bool found = false;
    while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytes_read, nullptr) && bytes_read > 0) {
        buffer[bytes_read] = '\0';
        if (strlen(buffer) > 0) {
            found = true;
            break;
        }
    }

    CloseHandle(hStdOutRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return found;
#else
    // Linux/macOS: 使用 which 命令
    std::string command = fmt::format("which {}", first_word);
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return false;

    char buffer[256];
    bool found = (fgets(buffer, sizeof(buffer), pipe) != nullptr);
    pclose(pipe);
    return found;
#endif
}

std::shared_ptr<Core::Tasks::Task> CLLILLMProvider::send_async(
    const LLMRequest& request,
    std::function<void(const LLMResponse&)> callback
) {
    auto task = Core::Tasks::TaskManager::instance().launch(
        "LLM CLI Request",
        [this, request, callback](const auto& cancel) {
            auto result = this->send(request);
            if (!cancel.is_cancelled()) {
                callback(result);
            }
        },
        Core::Tasks::TaskType::Background
    );

    return task;
}

Result<LLMResponse, std::string> CLLILLMProvider::send(const LLMRequest& request) {
    const auto start_time = std::chrono::steady_clock::now();

    try {
        // 构建命令
        const std::string command = build_command(request);

        // 执行命令
        auto output = execute_command(command);
        if (output.is_err()) {
            return LLMResponse::failure(output.unwrap_err());
        }

        LLMResponse response;
        response.content = output.unwrap();
        response.is_complete = true;

        const auto end_time = std::chrono::steady_clock::now();
        response.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time
        );

        return response;

    } catch (const std::exception& e) {
        return LLMResponse::failure(fmt::format("CLI execution failed: {}", e.what()));
    }
}

std::vector<std::string> CLLILLMProvider::get_models() const {
    // 返回常见的 CLI 模型
    return {"llama3.2", "qwen2.5", "mistral", "gemma", "deepseek-r1"};
}

std::string CLLILLMProvider::build_command(const LLMRequest& request) const {
    std::string command = m_command_template;

    // 替换占位符
    command = std::regex_replace(command, std::regex("\\{prompt\\}"), request.prompt);
    command = std::regex_replace(command, std::regex("\\{model\\}"), m_current_model);
    command = std::regex_replace(command, std::regex("\\{temperature\\}"), fmt::format("{:.2f}", request.temperature));
    command = std::regex_replace(command, std::regex("\\{max_tokens\\}"), std::format("{}", request.max_tokens));

    // 转义上下文消息
    if (!request.context.empty()) {
        // 将上下文合并到 prompt 中
        std::string full_prompt;
        for (const auto& msg : request.context) {
            full_prompt += msg + "\n";
        }
        full_prompt += request.prompt;
        command = std::regex_replace(command, std::regex("\\{prompt\\}"), full_prompt);
    }

    return command;
}

Result<std::string, std::string> CLLILLMProvider::execute_command(
    const std::string& command
) const {
#ifdef _WIN32
    // Windows: 使用 CreateProcess
    HANDLE hStdOutRead, hStdOutWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };

    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
        return Result<std::string, std::string>::err("Failed to create pipe");
    }

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;

    PROCESS_INFORMATION pi = {0};

    // 创建进程
    if (!CreateProcessA(
        nullptr,
        const_cast<char*>(command.c_str()),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    )) {
        CloseHandle(hStdOutRead);
        CloseHandle(hStdOutWrite);
        return Result<std::string, std::string>::err("Failed to create process");
    }

    CloseHandle(hStdOutWrite);

    // 读取输出
    std::string output;
    char buffer[4096];
    DWORD bytes_read;
    while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytes_read, nullptr) && bytes_read > 0) {
        buffer[bytes_read] = '\0';
        output += buffer;
    }
    CloseHandle(hStdOutRead);

    // 等待进程结束
    WaitForSingleObject(pi.hProcess, 60000); // 60秒超时
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return Result<std::string, std::string>::ok(output);

#else
    // Linux/macOS: 使用 popen
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return Result<std::string, std::string>::err("Failed to open process");
    }

    char buffer[4096];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        output += buffer;
    }

    const int exit_code = pclose(pipe);
    if (exit_code != 0) {
        return Result<std::string, std::string>::err(
            fmt::format("Process exited with code {}", exit_code)
        );
    }

    return Result<std::string, std::string>::ok(output);
#endif
}

} // namespace DearTs::Plugins::Chat::LLM
