# 日志系统

使用 liblogger 进行异步日志。

## 初始化

```cpp
Logger::init("logs/app.log", Logger::Level::Info);
```

## 使用

```cpp
LOG_INFO("Application started");
LOG_ERROR("Error: {}", errorMsg);
```

更多内容请参考项目文档。
