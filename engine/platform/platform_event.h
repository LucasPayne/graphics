#ifndef PLATFORM_EVENT_H_
#define PLATFORM_EVENT_H_
#include <stdint.h>

struct DisplayRefreshEvent
{
    double dt;
    double time;
    struct {
        uint16_t width;
        uint16_t height;
    } framebuffer;
};

enum WindowEventTypes
{
    WINDOW_EVENT_FRAMEBUFFER_SIZE,
};
struct WindowEvent
{
    uint8_t type;
    struct {
        uint16_t width;
        uint16_t height;
    } framebuffer;
};

enum KeyboardAction
{
    KEYBOARD_PRESS,
    KEYBOARD_RELEASE,
};

enum KeyboardKeys
{
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_Q,
    KEY_W,
    KEY_E,
    KEY_R,
    KEY_T,
    KEY_Y,
    KEY_U,
    KEY_I,
    KEY_O,
    KEY_P,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_Z,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_N,
    KEY_M,
    KEY_UP_ARROW,
    KEY_DOWN_ARROW,
    KEY_LEFT_ARROW,
    KEY_RIGHT_ARROW,
    KEY_LEFT_SHIFT,
    KEY_RIGHT_SHIFT,
    KEY_SPACE,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEYBOARD_NUM_KEYS
};
typedef uint8_t KeyboardKeyCode;

struct KeyboardKey
{
    KeyboardKeyCode code;
};

struct KeyboardEvent
{
    KeyboardKey key;
    uint8_t action;
};

struct CursorState
{
    double x;
    double y;
    double dx;
    double dy;
};

enum MouseAction
{
    MOUSE_BUTTON_PRESS,
    MOUSE_BUTTON_RELEASE,
    MOUSE_MOVE,
    MOUSE_SCROLL,
};

enum MouseButtons
{
    MOUSE_LEFT,
    MOUSE_RIGHT,
    MOUSE_MIDDLE,
    MOUSE_NUM_BUTTONS
};

typedef uint8_t MouseButtonCode;

struct MouseButton
{
    uint8_t code;
};

struct MouseEvent
{
    uint8_t action;
    MouseButton button;
    CursorState cursor; // No matter the event, this struct is filled.
    double scroll_y;
};

#endif // PLATFORM_EVENT_H_
