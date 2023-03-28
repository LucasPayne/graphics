#include "engine.h"
#include "platforms/glfw_vulkan_window.cc"
#include <stdio.h>

class Renderer
{
};

class Application : public PlatformListener
{
public:
    void keyboard_event_handler(KeyboardEvent e) override;
    void mouse_event_handler(MouseEvent e) override;
    void window_event_handler(WindowEvent e) override;
    void display_refresh_event_handler(DisplayRefreshEvent e) override;
    Application(Renderer &renderer) :
        m_renderer{renderer}
    {
    }
private:
    Renderer &m_renderer;
};

void Application::keyboard_event_handler(KeyboardEvent e)
{
}
void Application::mouse_event_handler(MouseEvent e)
{
}
void Application::window_event_handler(WindowEvent e)
{
}
void Application::display_refresh_event_handler(DisplayRefreshEvent e)
{
    printf("%.2f\n", e.dt);
}

int main()
{
    Renderer renderer;
    Application app(renderer);

    std::unique_ptr<Platform> platform = Platform_GLFWVulkanWindow::create();
    platform->add_listener(&app);
    platform->enter_loop();
}
