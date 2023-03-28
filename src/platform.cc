#include "platform.h"

void Platform::add_listener(PlatformListener *listener)
{
    m_listeners.push_back(listener);
}

void Platform::set_display_deltatime(double display_deltatime)
{
    m_display_deltatime = display_deltatime;
}

void Platform::emit_keyboard_event(KeyboardEvent e)
{
    for (PlatformListener *listener : m_listeners)
    {
        listener->keyboard_event_handler(e);
    }
}

void Platform::emit_mouse_event(MouseEvent e)
{
    for (PlatformListener *listener : m_listeners)
    {
        listener->mouse_event_handler(e);
    }
}
