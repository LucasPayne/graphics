#include "platform.h"
#include "platforms/glfw_vulkan_window.cc"

class Application : public PlatformListener
{
public:
    void keyboard_event_handler(KeyboardEvent e) override;
    void mouse_event_handler(MouseEvent e) override;
    Application()
    {
    };
};

void Application::keyboard_event_handler(KeyboardEvent e)
{
}
void Application::mouse_event_handler(MouseEvent e)
{
}

int main()
{
    std::unique_ptr<Platform> platform = Platform_GLFWVulkanWindow::create();
    Application app;
    platform->add_listener(&app);
    platform->enter_loop();
}
