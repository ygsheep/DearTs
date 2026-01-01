# 任务系统

异步任务管理。

## 创建任务

```cpp
auto task = TaskManager::instance().createTask("Task Name", [](Task& t) {
    // 任务逻辑
});
```

更多内容请参考项目文档。
