# Application 类 API

核心应用程序基类。

## 生命周期

```cpp
class MyApp : public dearts::Application {
    bool onInitialize() override;
    void onUpdate(float deltaTime) override;
    void onRender() override;
    void onEvent(SDL_Event& event) override;
    void onShutdown() override;
};
```

更多内容请参考项目文档。
