# 插件系统

DearTs 支持插件扩展。

## 创建插件

```cpp
class MyPlugin : public Plugin {
    void onLoad() override;
    void onUnload() override;
};

REGISTER_PLUGIN(MyPlugin, "1.0.0");
```

更多内容请参考项目文档。
