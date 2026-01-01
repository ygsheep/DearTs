# Result 类型详解

DearTs Framework 使用 `Result<T, E>` 类型进行统一的错误处理，替代传统的异常或返回值方式。

## 概述

**文件**: `core/result.h`

`Result<T, E>` 是一个类型安全的错误处理容器，类似于 Rust 的 `Result<T, E>` 或 C++23 的 `std::expected<T, E>`。

## 基本用法

### 创建 Result

```cpp
// 成功值
Result<int, std::string> divide(int a, int b) {
    if (b == 0) {
        return Result::err("Division by zero");
    }
    return Result::ok(a / b);
}

// void 特化版本
Result<void, std::string> saveFile(const std::string& path) {
    if (!writeFile(path)) {
        return Result::err("Failed to write file");
    }
    return Result::ok();
}
```

### 检查和提取值

```cpp
auto result = divide(10, 2);

if (result.is_ok()) {
    int value = result.unwrap();
    LOG_INFO("Result: {}", value);
} else {
    std::string error = result.unwrap_err();
    LOG_ERROR("Error: {}", error);
}
```

## 函数式编程

### map - 转换值

```cpp
auto result = divide(10, 2)
    .map([](int value) {
        return value * 2;
    });  // Result<int, std::string>

// 如果包含错误，map 不会执行
auto error_result = divide(10, 0)
    .map([](int value) {
        return value * 2;  // 不会执行
    });  // Result<int, std::string> (仍然包含错误)
```

### and_then - 链式调用

```cpp
auto result = divide(10, 2)
    .and_then([](int value) {
        return saveToFile(value);
    })
    .and_then([](auto) {
        return showSuccessMessage();
    });

// 任何一个步骤返回错误，后续步骤不会执行
```

### or_else - 错误处理

```cpp
auto result = divide(10, 2)
    .or_else([](const std::string& error) {
        LOG_ERROR("Operation failed: {}", error);
        return Result::ok(0);  // 提供默认值
    });
```

### transform - 同时处理成功和错误

```cpp
auto result = divide(10, 2)
    .transform(
        [](int value) { return "Success: " + std::to_string(value); },
        [](const std::string& error) { return "Error: " + error; }
    );  // 返回 std::string
```

## 高级用法

### 自定义错误类型

```cpp
enum class ErrorCode {
    DivisionByZero,
    Overflow,
    InvalidInput
};

Result<int, ErrorCode> safeDivide(int a, int b) {
    if (b == 0) return Result::err(ErrorCode::DivisionByZero);
    if (a == INT_MIN && b == -1) return Result::err(ErrorCode::Overflow);
    return Result::ok(a / b);
}

auto result = safeDivide(10, 2);
if (result.is_err()) {
    ErrorCode error = result.unwrap_err();
    switch (error) {
        case ErrorCode::DivisionByZero:
            // 处理除零错误
            break;
        case ErrorCode::Overflow:
            // 处理溢出错误
            break;
    }
}
```

### 链式操作

```cpp
Result<Image, std::string> loadImageAndProcess(const std::string& path) {
    return loadImage(path)
        .map([](Image img) {
            return img.resize(800, 600);
        })
        .and_then([](Image img) {
            return img.applyFilter("blur");
        })
        .map([](Image img) {
            return img.convertToGrayscale();
        });
}
```

### 组合多个 Result

```cpp
Result<std::tuple<int, int, int>, std::string>
parseThreeNumbers(const std::vector<std::string>& strs) {
    return parseNumber(strs[0])
        .and_then([&](int a) {
            return parseNumber(strs[1])
                .map([&](int b) {
                    return parseNumber(strs[2])
                        .map([&](int c) {
                            return std::make_tuple(a, b, c);
                        });
                });
        });
}
```

## 实际应用示例

### 文件操作

```cpp
Result<std::string, std::string> readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return Result::err("Failed to open file: " + path);
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return Result::ok(content);
}

Result<void, std::string> writeFile(const std::string& path,
                                     const std::string& content) {
    std::ofstream file(path);
    if (!file) {
        return Result::err("Failed to open file: " + path);
    }

    file << content;
    if (!file) {
        return Result::err("Failed to write to file: " + path);
    }

    return Result::ok();
}

// 使用
auto result = readFile("data.txt")
    .and_then([](const std::string& data) {
        return process(data);
    })
    .and_then([](const std::string& processed) {
        return writeFile("output.txt", processed);
    });

if (result.is_err()) {
    LOG_ERROR("Error: {}", result.unwrap_err());
}
```

### 网络请求

```cpp
enum class HttpError {
    ConnectionFailed,
    Timeout,
    InvalidResponse
};

Result<nlohmann::json, HttpError> fetchJson(const std::string& url) {
    auto response = httpClient.get(url);
    if (!response) {
        return Result::err(HttpError::ConnectionFailed);
    }

    if (response.status != 200) {
        return Result::err(HttpError::InvalidResponse);
    }

    try {
        auto json = nlohmann::json::parse(response.body);
        return Result::ok(json);
    } catch (...) {
        return Result::err(HttpError::InvalidResponse);
    }
}

// 使用
auto result = fetchJson("https://api.example.com/data")
    .map([](const nlohmann::json& json) {
        return json["value"].get<int>();
    })
    .or_else([](HttpError error) {
        LOG_ERROR("HTTP Error: {}", static_cast<int>(error));
        return Result::ok(0);  // 默认值
    });
```

## 与异常的对比

### 传统异常方式

```cpp
// ❌ 不推荐
int divide(int a, int b) {
    if (b == 0) {
        throw std::runtime_error("Division by zero");
    }
    return a / b;
}

// 使用
try {
    int result = divide(10, 0);
} catch (const std::exception& e) {
    LOG_ERROR("Error: {}", e.what());
}
```

### Result 方式

```cpp
// ✅ 推荐
Result<int, std::string> divide(int a, int b) {
    if (b == 0) {
        return Result::err("Division by zero");
    }
    return Result::ok(a / b);
}

// 使用
auto result = divide(10, 0);
if (result.is_err()) {
    LOG_ERROR("Error: {}", result.unwrap_err());
}
```

## 性能考虑

### 编译时优化

`Result<T, E>` 是零开销抽象：

- 大小等于 `sizeof(T) + sizeof(E) + 1 byte` (状态标志)
- 无虚函数调用
- 无堆分配（除非 T 或 E 需要）
- 编译器可以优化掉未使用的错误路径

### 移动语义

```cpp
Result<std::string, std::string> createString() {
    return Result::ok(std::string("Hello"));  // 移动构造
}

// 避免拷贝
auto result = createString();
std::string s = std::move(result).unwrap();  // 移动提取
```

## 最佳实践

### 1. 错误类型应包含足够信息

```cpp
// ❌ 不好
Result<int, bool> parseNumber(std::string_view s);

// ✅ 好
Result<int, ParseError> parseNumber(std::string_view s);
// ParseError 包含错误位置、错误类型等
```

### 2. 使用强类型错误

```cpp
// ❌ 不好
Result<int, std::string> readFile(std::string_view path);

// ✅ 好
enum class FileError {
    NotFound,
    PermissionDenied,
    Corruption
};
Result<std::string, FileError> readFile(std::string_view path);
```

### 3. 优先使用 `and_then` 而嵌套 if

```cpp
// ❌ 不好
auto r1 = operation1();
if (r1.is_ok()) {
    auto r2 = operation2(r1.unwrap());
    if (r2.is_ok()) {
        auto r3 = operation3(r2.unwrap());
        // ...
    }
}

// ✅ 好
auto result = operation1()
    .and_then([](auto v1) { return operation2(v1); })
    .and_then([](auto v2) { return operation3(v2); });
```

### 4. 使用 `map` 进行值转换

```cpp
// ✅ 好
auto result = parseNumber(input)
    .map([](int value) { return value * 2; });
```

## API 参考

### 类型定义

```cpp
template<typename T, typename E = std::string>
class Result {
public:
    // 构造函数
    Result(const Result&) = delete;
    Result(Result&&) noexcept;
    Result& operator=(const Result&) = delete;
    Result& operator=(Result&&) noexcept;
    ~Result();

    // 工厂函数
    static Result ok(T value);
    static Result err(E error);

    // 查询
    bool is_ok() const;
    bool is_err() const;
    explicit operator bool() const;

    // 提取值
    T& unwrap();
    const T& unwrap() const;
    T unwrap_or(T&& defaultValue) const;
    E& unwrap_err();
    const E& unwrap_err() const;

    // 函数式操作
    template<typename F>
    auto map(F&& f) -> Result<std::invoke_result_t<F, T>, E>;

    template<typename F>
    auto and_then(F&& f) -> /* ... */;

    template<typename F>
    auto or_else(F&& f) -> /* ... */;

    template<typename F1, typename F2>
    auto transform(F1&& ok_func, F2&& err_func) -> /* ... */;
};
```

## 参考资源

- Rust Result 文档: https://doc.rust-lang.org/std/result/enum.Result.html
- C++23 std::expected: https://en.cppreference.com/w/cpp/utility/expected
- 源码: `core/result.h`
