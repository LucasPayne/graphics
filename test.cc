
EngineWindow

class Engine
{
public:
private:
    EnginePlatform *m_platform;
    bool m_platform_initialized;

    std::vector<EngineWindow> m_windows;
}

void EngineWindow::enter_loop(Application &app)
{
    while (true)
    {
        platform.make_current();
    }
}

void Engine::new_window()
{
    // Lazy initialize the platform backend.
    if (!m_platform_initialized)
    {
        m_platform.init();
        m_platform_initialized = true;
    }
}

struct Platform
{
    virtual bool init() = 0;
    virtual bool make_current() = 0;
    virtual bool present() = 0;
}

int main()
{
    Engine engine;
    engine.set_platform(platform_glfw_vulkan);
    EngineWindow *window = engine.new_window(app);
    window->enter_loop(app);

    platform_glfw_vulkan__init();
}
