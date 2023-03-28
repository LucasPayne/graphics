#ifndef PLATFORM_H_
#define PLATFORM_H_
#include <vector>
#include "platform_event.h"

class PlatformListener
{
public:
    virtual void keyboard_event_handler(KeyboardEvent e)
    {}
    virtual void mouse_event_handler(MouseEvent e)
    {}
    virtual void window_event_handler(WindowEvent e)
    {}
    virtual void display_refresh_event_handler(DisplayRefreshEvent e)
    {}
};

class Platform
{
public:
    void add_listener(PlatformListener *listener);
    virtual void enter_loop() = 0;
//Would prefer to be protected, but glfw callbacks need access.
//protected:
    void set_display_deltatime(double display_deltatime);
    void emit_keyboard_event(KeyboardEvent e);
    void emit_mouse_event(MouseEvent e);
    void emit_window_event(WindowEvent e);
    void emit_display_refresh_event(DisplayRefreshEvent e);

    std::vector<PlatformListener *> m_listeners;
private:
    double m_display_deltatime;
};

#endif // PLATFORM_H_

