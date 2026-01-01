// 插件模板
class MyPlugin : public dearts::Plugin {
public:
    std::string getName() const override {
        return "My Plugin";
    }

    void onLoad() override {
        // 注册命令、工具等
    }

    void onUnload() override {
        // 清理资源
    }
};

REGISTER_PLUGIN(MyPlugin, "1.0.0");
