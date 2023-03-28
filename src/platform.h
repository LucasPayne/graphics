#ifndef PLATFORM_H_
#define PLATFORM_H_
#include <vector>
#include "platform_input.h"

class PlatformListener
{
public:
    virtual void keyboard_event_handler(KeyboardEvent e) = 0;
    virtual void mouse_event_handler(MouseEvent e) = 0;
};

class Platform
{
public:
    void add_listener(PlatformListener *listener);
    virtual void enter_loop() = 0;
protected:
    //void set_graphics_api(VkSystem &vk_system);
    //void make_graphics_api_current();
    void set_display_deltatime(double display_deltatime);
    void emit_keyboard_event(KeyboardEvent e);
    void emit_mouse_event(MouseEvent e);

    std::vector<PlatformListener *> m_listeners;
private:
    double m_display_deltatime;
};

#endif // PLATFORM_H_

