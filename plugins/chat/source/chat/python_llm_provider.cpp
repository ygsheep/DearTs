/**
 * @file python_llm_provider.cpp
 * @brief Python LLM 提供商实现
 */

#include "chat/llm/python_llm_provider.hpp"
#include "liblogger/logger.h"
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/wait.h>
#endif

namespace DearTs::Plugins::Chat::LLM {

using json = nlohmann::json;

PythonLLMProvider::PythonLLMProvider(
    const std::string& script_path,
    const std::string& python_path
) : m_script_path(script_path)
  , m_python_path(python_path) {
}

bool PythonLLMProvider::is_available() const {
#ifdef _WIN32
    // 检查 Python 是否可用
    std::string command = m_python_path + " --version";
    HANDLE hStdOutRead, hStdOutWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };

    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
        return false;
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
        return false;
    }

    // 等待进程结束
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(hStdOutWrite);

    // 读取输出
    char buffer[256];
    DWORD bytes_read;
    std::string output;
    while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytes_read, nullptr) && bytes_read > 0) {
        buffer[bytes_read] = '\0';
        output += buffer;
    }

    CloseHandle(hStdOutRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // 检查输出是否包含 Python 版本
    return output.find("Python") != std::string::npos;
#else
    // 使用 which 检查 Python
    std::string command = fmt::format("which {}", m_python_path);
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return false;

    char buffer[256];
    if (fgets(buffer, sizeof(buffer), pipe)) {
        pclose(pipe);
        return true;
    }
    pclose(pipe);
    return false;
#endif
}

std::shared_ptr<Core::Tasks::Task> PythonLLMProvider::send_async(
    const LLMRequest& request,
    std::function<void(const LLMResponse&)> callback
) {
    auto task = Core::Tasks::TaskManager::instance().launch(
        "LLM Python Request",
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

Result<LLMResponse, std::string> PythonLLMProvider::send(const LLMRequest& request) {
    const auto start_time = std::chrono::steady_clock::now();

    try {
        // 构建输入 JSON
        json j;
        j["prompt"] = request.prompt;
        j["context"] = request.context;
        j["system_prompt"] = request.system_prompt;
        j["temperature"] = request.temperature;
        j["max_tokens"] = request.max_tokens;
        j["model"] = m_current_model;

        const std::string json_input = j.dump();

        // 执行 Python 脚本
        auto output = execute_python_script(json_input);
        if (output.is_err()) {
            return LLMResponse::failure(output.unwrap_err());
        }

        // 解析输出
        json response_json = json::parse(output.unwrap());

        LLMResponse response;
        response.content = response_json["content"].get<std::string>();
        response.is_complete = response_json["is_complete"].get<bool>();

        if (response_json.contains("tokens_used")) {
            response.tokens_used = response_json["tokens_used"].get<int>();
        }

        const auto end_time = std::chrono::steady_clock::now();
        response.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time
        );

        return response;

    } catch (const std::exception& e) {
        return LLMResponse::failure(fmt::format("Python execution failed: {}", e.what()));
    }
}

std::vector<std::string> PythonLLMProvider::get_models() const {
    // 返回常见的本地模型
    return {"llama3.2", "qwen2.5", "deepseek-r1", "mistral", "gemma"};
}

Result<std::string, std::string> PythonLLMProvider::execute_python_script(
    const std::string& json_input
) const {
#ifdef _WIN32
    // Windows: 使用 CreateProcess 和管道
    HANDLE hStdInRead, hStdInWrite;
    HANDLE hStdOutRead, hStdOutWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };

    // 创建管道
    if (!CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0) ||
        !CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
        return Result<std::string, std::string>::err("Failed to create pipes");
    }

    // 设置命令行
    std::string command = fmt::format("{} \"{}\"", m_python_path, m_script_path);

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hStdInRead;
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
        CloseHandle(hStdInRead);
        CloseHandle(hStdInWrite);
        CloseHandle(hStdOutRead);
        CloseHandle(hStdOutWrite);
        return Result<std::string, std::string>::err("Failed to create Python process");
    }

    // 关闭不需要的句柄
    CloseHandle(hStdInRead);
    CloseHandle(hStdOutWrite);

    // 写入输入
    DWORD bytes_written;
    if (!WriteFile(hStdInWrite, json_input.data(), json_input.length(), &bytes_written, nullptr)) {
        CloseHandle(hStdInWrite);
        CloseHandle(hStdOutRead);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return Result<std::string, std::string>::err("Failed to write to Python process");
    }
    CloseHandle(hStdInWrite);

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
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return Result<std::string, std::string>::ok(output);

#else
    // Linux/macOS: 使用 popen
    std::string command = fmt::format("{} \"{}\"", m_python_path, m_script_path);
    FILE* pipe = popen(command.c_str(), "w");
    if (!pipe) {
        return Result<std::string, std::string>::err("Failed to open Python process");
    }

    // 写入输入
    if (fputs(json_input.c_str(), pipe) == EOF) {
        pclose(pipe);
        return Result<std::string, std::string>::err("Failed to write to Python process");
    }

    // 读取输出
    char buffer[4096];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        output += buffer;
    }

    const int exit_code = pclose(pipe);
    if (exit_code != 0) {
        return Result<std::string, std::string>::err(
            fmt::format("Python process exited with code {}", exit_code)
        );
    }

    return Result<std::string, std::string>::ok(output);
#endif
}

} // namespace DearTs::Plugins::Chat::LLM
