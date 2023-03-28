#include "platform.h"
#include "platform/glfw_vulkan_window.cc"

class Application : public PlatformListener
{
public:
    void keyboard_event_handler(KeyboardEvent e) = 0;
    void mouse_event_handler(MouseEvent e) = 0;
    Application()
    {
    };
}

int main()
{
    std::unique_ptr<Platform> platform = Platform_GLFWVulkanWindow::create();
    Application app;
    platform->add_listener(&app);
    platform->enter_loop();
}
