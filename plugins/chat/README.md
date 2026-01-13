# ChatManager - 基于 DearTs Framework 的聊天 GUI 系统

## 项目概述

ChatManager 是一个现代化的聊天 GUI 应用程序，基于 DearTs Framework 开发，具有 AI 智能辅助功能，支持接入本地大模型（如 Ollama）和未来扩展微信自动化。

## 架构设计

```
DearTs Framework Core
├── EventBus（类型安全事件系统）
├── TaskManager（异步任务管理）
├── ConfigManager（配置持久化）
└── View System（停靠窗口）

┌─────────────┐     ┌──────────────┐
│DearTs 主应用│     │ChatManager 应用│
│ (所有插件)  │     │ (仅 chat 插件) │
└─────────────┘     └──────────────┘
                            │
                            ▼
                    ┌────────────────┐
                    │  Chat Plugin   │
                    │   plugins/chat/ │
                    └────────────────┘
```

## 功能特性

### 核心功能
- ✅ **三栏布局**：会话列表 | 聊天区域 | 信息面板
- ✅ **现代气泡**：圆角消息气泡，用户/AI 不同样式
- ✅ **AI 智能辅助**：后台分析，提供多个回复建议
- ✅ **多 LLM 支持**：HTTP API / Python 脚本 / 命令行工具
- ✅ **虚拟滚动**：支持大量消息流畅渲染
- ✅ **会话管理**：创建、删除、搜索会话
- ✅ **导出功能**：JSON / Markdown / TXT / HTML

### LLM Provider 支持

| Provider | 说明 | 示例 |
|----------|------|------|
| HTTP | OpenAI 兼容 API | Ollama, LocalAI, OpenAI |
| Python | Python 脚本调用 | transformers, llama-cpp-python |
| CLI | 命令行工具 | llama.cpp, ollama CLI |

## 项目结构

```
DearTs/
├── plugins/chat/                          # Chat 插件
│   ├── CMakeLists.txt
│   ├── include/chat/
│   │   ├── chat_plugin.hpp
│   │   ├── models/
│   │   │   ├── message.hpp
│   │   │   ├── conversation.hpp
│   │   │   └── ai_suggestion.hpp
│   │   ├── events/
│   │   │   └── chat_events.hpp
│   │   ├── views/
│   │   │   ├── conversation_list_view.hpp
│   │   │   ├── chat_view.hpp
│   │   │   └── info_panel_view.hpp
│   │   ├── llm/
│   │   │   ├── llm_interface.hpp
│   │   │   ├── http_llm_provider.hpp
│   │   │   ├── python_llm_provider.hpp
│   │   │   └── cli_llm_provider.hpp
│   │   └── ui/
│   │       ├── message_bubble.hpp
│   │       ├── suggestion_chip.hpp
│   │       └── chat_input.hpp
│   └── source/chat/
│       ├── chat_plugin.cpp
│       ├── models/
│       ├── views/
│       ├── llm/
│       └── ui/
│
└── main/chatmanager/                       # ChatManager 应用
    ├── CMakeLists.txt
    ├── include/chatmanager/
    │   └── application.hpp
    └── source/
        └── main.cpp
```

## 构建说明

### 前置要求

- CMake 3.20+
- C++20 编译器（MSVC 2022, GCC 11+, Clang 13+）
- SDL3
- ImGui 2.13.3+
- nlohmann/json
- fmtlib

### 构建步骤

```bash
# 配置构建
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build build --config Release

# 运行
./build/bin/ChatManager.exe
```

## 使用说明

### 启动应用

1. 启动 ChatManager 应用
2. 默认会创建一个"新对话"会话
3. 在输入框中输入消息，按 Enter 发送

### AI 配置

1. 在右侧"AI 设置"标签中配置 LLM
2. 选择 Provider：
   - **HTTP**：适用于 Ollama 等服务（默认地址：http://localhost:11434/v1）
   - **Python**：需要配置 Python 脚本路径
   - **CLI**：需要配置命令行工具模板

3. 选择模型并调整参数（temperature、max_tokens）

### 快捷键

- `Ctrl+N`：新建会话
- `Enter`：发送消息
- `Shift+Enter`：换行

## 配置文件

配置保存在 `chatmanager.json`：

```json
{
  "chat": {
    "llm": {
      "provider": "http",
      "http": {
        "base_url": "http://localhost:11434/v1",
        "api_key": "",
        "model": "llama3.2"
      },
      "parameters": {
        "temperature": 0.7,
        "max_tokens": 2048
      }
    },
    "ui": {
      "bubble_style": "modern",
      "auto_scroll": true
    },
    "ai_assistant": {
      "enabled": true,
      "suggestion_count": 3
    }
  }
}
```

## Python 脚本示例

创建 `scripts/chat_llm.py`：

```python
import sys
import json

def generate_response(prompt, context):
    """使用本地 LLM 生成响应"""
    # TODO: 实现你的 LLM 调用逻辑
    # 例如使用 transformers、llama-cpp-python 等
    return "这是 AI 的回复"

if __name__ == "__main__":
    # 从 stdin 读取请求
    request = json.loads(sys.stdin.read())

    # 生成响应
    response = generate_response(
        request['prompt'],
        request.get('context', [])
    )

    # 输出响应
    result = {
        "content": response,
        "is_complete": True,
        "tokens_used": 100
    }
    print(json.dumps(result))
```

## Ollama 集成

### 安装 Ollama

```bash
# 下载 Ollama
# https://ollama.ai/

# 拉取模型
ollama pull llama3.2

# 运行服务
ollama serve
```

### 配置 ChatManager

1. Provider 选择：`HTTP`
2. Base URL：`http://localhost:11434/v1`
3. Model：`llama3.2`

## 开发指南

### 创建自定义 LLM Provider

继承 `ILLMProvider` 接口：

```cpp
class MyLLMProvider : public ILLMProvider {
public:
    std::string get_name() const override { return "MyProvider"; }
    bool is_available() const override { /* ... */ }

    Result<LLMResponse, std::string> send(const LLMRequest& request) override {
        // 实现你的 LLM 调用逻辑
        return LLMResponse::success("Response");
    }

    // ...
};
```

### 添加新的事件

在 `chat_events.hpp` 中定义：

```cpp
struct MyCustomEvent {
    std::string data;
};
```

订阅事件：

```cpp
EventBus::instance().subscribe<MyCustomEvent>(
    [](const MyCustomEvent& e) {
        // 处理事件
    }
);
```

## 扩展计划

### Phase 1：基础功能 ✅
- [x] 三栏布局
- [x] 消息气泡
- [x] LLM 集成
- [x] 会话管理

### Phase 2：增强功能（待实现）
- [ ] 流式响应显示
- [ ] 多模态支持（图片、文件）
- [ ] 语音输入/输出
- [ ] 代码高亮

### Phase 3：微信集成（未来）
- [ ] 微信协议适配
- [ ] 消息收发
- [ ] 联系人同步
- [ ] 群聊支持

## 技术栈

- **框架**：DearTs Framework (SDL3 + ImGui)
- **语言**：C++20
- **构建**：CMake
- **JSON**：nlohmann/json
- **日志**：spdlog/fmt

## 许可证

MIT License

## 贡献

欢迎提交 Issue 和 Pull Request！

---

**分支**：`feature/chatmanager`
**状态**：开发中
