#define GLFW_INCLUDE_VULKAN

#include "engine/engine.h"
#include "engine/platform/vk.h"

#include <GLFW/glfw3.h>
#ifndef GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#endif

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <vector>
#include <limits>
#include <optional>
#include <set>
#include <algorithm>
#include <memory>

#include "ansi_color.h"

#include <json/json.h>

static int glfw_keycode_to_keycode(int key)
{
    switch (key) {
        case GLFW_KEY_1: return KEY_1;
        case GLFW_KEY_2: return KEY_2;
        case GLFW_KEY_3: return KEY_3;
        case GLFW_KEY_4: return KEY_4;
        case GLFW_KEY_5: return KEY_5;
        case GLFW_KEY_6: return KEY_6;
        case GLFW_KEY_7: return KEY_7;
        case GLFW_KEY_8: return KEY_8;
        case GLFW_KEY_9: return KEY_9;
        case GLFW_KEY_0: return KEY_0;
        case GLFW_KEY_Q: return KEY_Q;
        case GLFW_KEY_W: return KEY_W;
        case GLFW_KEY_E: return KEY_E;
        case GLFW_KEY_R: return KEY_R;
        case GLFW_KEY_T: return KEY_T;
        case GLFW_KEY_Y: return KEY_Y;
        case GLFW_KEY_U: return KEY_U;
        case GLFW_KEY_I: return KEY_I;
        case GLFW_KEY_O: return KEY_O;
        case GLFW_KEY_P: return KEY_P;
        case GLFW_KEY_A: return KEY_A;
        case GLFW_KEY_S: return KEY_S;
        case GLFW_KEY_D: return KEY_D;
        case GLFW_KEY_F: return KEY_F;
        case GLFW_KEY_G: return KEY_G;
        case GLFW_KEY_H: return KEY_H;
        case GLFW_KEY_J: return KEY_J;
        case GLFW_KEY_K: return KEY_K;
        case GLFW_KEY_L: return KEY_L;
        case GLFW_KEY_Z: return KEY_Z;
        case GLFW_KEY_X: return KEY_X;
        case GLFW_KEY_C: return KEY_C;
        case GLFW_KEY_V: return KEY_V;
        case GLFW_KEY_B: return KEY_B;
        case GLFW_KEY_N: return KEY_N;
        case GLFW_KEY_M: return KEY_M;
        case GLFW_KEY_UP: return KEY_UP_ARROW;
        case GLFW_KEY_DOWN: return KEY_DOWN_ARROW;
        case GLFW_KEY_LEFT: return KEY_LEFT_ARROW;
        case GLFW_KEY_RIGHT: return KEY_RIGHT_ARROW;
        case GLFW_KEY_LEFT_SHIFT: return KEY_LEFT_SHIFT;
        case GLFW_KEY_RIGHT_SHIFT: return KEY_RIGHT_SHIFT;
        case GLFW_KEY_SPACE: return KEY_SPACE;
        case GLFW_KEY_PAGE_UP: return KEY_PAGE_UP;
        case GLFW_KEY_PAGE_DOWN: return KEY_PAGE_DOWN;
    }
    return EOF;
}


class Platform_GLFWVulkanWindow : public Platform
{
public:
    static std::unique_ptr<Platform_GLFWVulkanWindow> create();
    void enter_loop() override;

    static Platform_GLFWVulkanWindow *Get()
    {
        return m_active;
    }

    VulkanSystem *GetVulkanSystem()
    {
        return &vk_system;
    }
private:
    GLFWwindow *glfw_window;
    VulkanSystem vk_system;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;

    static Platform_GLFWVulkanWindow *m_active;
    Platform_GLFWVulkanWindow();
};
Platform_GLFWVulkanWindow *Platform_GLFWVulkanWindow::m_active = nullptr;

Platform_GLFWVulkanWindow::Platform_GLFWVulkanWindow()
{
}


void glfw_key_callback(GLFWwindow *window, int key,
                       int scancode, int action,
                       int mods)
{
    KeyboardEvent e;
    int c;
    if ((c = glfw_keycode_to_keycode(key)) == EOF) return;
    e.key.code = (KeyboardKeyCode) c;
    
    if (action == GLFW_PRESS) {
        e.action = KEYBOARD_PRESS;
    } else if (action == GLFW_RELEASE) {
        e.action = KEYBOARD_RELEASE;
    } else {
        return; // Action not handled, no-op.
    }
    Platform_GLFWVulkanWindow::Get()->emit_keyboard_event(e);
}


static CursorState g_glfw_cursor = { 0 };
static bool g_glfw_cursor_initialized = false;
void glfw_cursor_position_callback(GLFWwindow *window, double window_x, double window_y)
{
    // (0,0) bottom-left of window, (1,1) top-right.
    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);
    double x = window_x / (1.0 * window_width);
    double y = 1 - window_y / (1.0 * window_height);

    if (!g_glfw_cursor_initialized)
    {
        g_glfw_cursor.x = x;
        g_glfw_cursor.x = y;
        g_glfw_cursor_initialized = true;
    }
    g_glfw_cursor.dx = x - g_glfw_cursor.x;
    g_glfw_cursor.dy = y - g_glfw_cursor.y;
    g_glfw_cursor.x = x;
    g_glfw_cursor.y = y;

    MouseEvent e;
    e.action = MOUSE_MOVE;
    e.cursor = g_glfw_cursor;

    Platform_GLFWVulkanWindow::Get()->emit_mouse_event(e);
}


void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    MouseEvent e;
    switch (action) {
        case GLFW_PRESS: e.action = MOUSE_BUTTON_PRESS; break;
        case GLFW_RELEASE: e.action = MOUSE_BUTTON_RELEASE; break;
        default:
            return; // Action not accounted for, no-op.
    }
    if (action == GLFW_PRESS) e.action = MOUSE_BUTTON_PRESS;
    else if (action == GLFW_PRESS) e.action = MOUSE_BUTTON_PRESS;
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT: e.button.code = MOUSE_LEFT; break;
        case GLFW_MOUSE_BUTTON_RIGHT: e.button.code = MOUSE_RIGHT; break;
        case GLFW_MOUSE_BUTTON_MIDDLE: e.button.code = MOUSE_MIDDLE; break;
        default:
            return; // Button not accounted for, no-op.
    }
    assert(g_glfw_cursor_initialized);
    e.cursor = g_glfw_cursor;
    Platform_GLFWVulkanWindow::Get()->emit_mouse_event(e);
}


void glfw_scroll_callback(GLFWwindow* window, double x_offset, double y_offset)
{
    MouseEvent e;
    e.action = MOUSE_SCROLL;
    e.scroll_y = y_offset;
    Platform_GLFWVulkanWindow::Get()->emit_mouse_event(e);
}

static void glfw_framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    WindowEvent e;
    e.type = WINDOW_EVENT_FRAMEBUFFER_SIZE;
    e.framebuffer.width = width;
    e.framebuffer.height = height;
    Platform_GLFWVulkanWindow::Get()->emit_window_event(e);
}


std::unique_ptr<Platform_GLFWVulkanWindow> Platform_GLFWVulkanWindow::create()
{
    assert(!m_active);

    if ( !glfwInit() )
    {
        fprintf(stderr, C_RED  "[GLFW] Failed to initialize glfw.\n" C_RESET);
        return nullptr;
    }

    if ( !glfwVulkanSupported() )
    {
        fprintf(stderr, C_RED  "[GLFW] Vulkan is not supported (Is the loader installed correctly?)\n" C_RESET);
        return nullptr;
    }

    std::unique_ptr<Platform_GLFWVulkanWindow> platform = std::unique_ptr<Platform_GLFWVulkanWindow>(new Platform_GLFWVulkanWindow());

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *video_mode = glfwGetVideoMode(monitor);
    int monitor_width = video_mode->width;
    int monitor_height = video_mode->height;
    int monitor_x, monitor_y;
    glfwGetMonitorPos(monitor, &monitor_x, &monitor_y);
    int window_x = monitor_x + 3*monitor_width/4;
    int window_y = monitor_y + 3*monitor_height/10;
    int window_width = monitor_width - 3*monitor_width/4 - 5;
    int window_height = (4*monitor_height)/10;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE); //todo: Not working, at least with i3?
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE); //todo: Not working, at least with i3?
    GLFWwindow *glfw_window = glfwCreateWindow(window_width, window_height, "graphics", nullptr, nullptr);
    if (glfw_window == nullptr)
    {
        fprintf(stderr, C_RED "[GLFW] Failed to create window.\n" C_RESET);
        return nullptr;
    }
    platform->glfw_window = glfw_window;
    glfwSetWindowPos(glfw_window, window_x, window_y);
    // Input and event callbacks.
    glfwSetKeyCallback(glfw_window, glfw_key_callback);
    glfwSetMouseButtonCallback(glfw_window, glfw_mouse_button_callback);
    glfwSetCursorPosCallback(glfw_window, glfw_cursor_position_callback);
    glfwSetScrollCallback(glfw_window, glfw_scroll_callback);
    glfwSetFramebufferSizeCallback(glfw_window, glfw_framebuffer_size_callback);

    std::vector<std::string> extra_layers = {
        //"VK_LAYER_KHRONOS_validation"
    };
    std::vector<std::string> extra_instance_extensions = {
    };
    std::vector<std::string> extra_device_extensions = {
    };
    // Add glfw's required extensions.
    {
        uint32_t num_glfw_instance_extensions;
        const char **glfw_instance_extensions = glfwGetRequiredInstanceExtensions( &num_glfw_instance_extensions );
        for ( uint32_t i = 0; i < num_glfw_instance_extensions; i++)
            extra_instance_extensions.emplace_back( glfw_instance_extensions[i] );
    }

    auto create_surface = [glfw_window](VkInstance vk_instance,
                                        VkPhysicalDevice vk_physical_device,
                                        VulkanSystemCreateSurfaceOutput *created_surface)->bool
    {
        VkSurfaceKHR vk_surface;
        VkResult err = glfwCreateWindowSurface(vk_instance, glfw_window, NULL, &vk_surface);
        if ( err != VK_SUCCESS ) return false;

        int width, height;
        glfwGetFramebufferSize(glfw_window, &width, &height);

        created_surface->surface = vk_surface;
        created_surface->initial_framebuffer_pixel_width = (uint32_t) width;
        created_surface->initial_framebuffer_pixel_height = (uint32_t) height;
        return true;
    };
    VulkanSystem vk_system;
    if ( !CreateVulkanSystem(&vk_system,
                             create_surface,
                             extra_layers,
                             extra_instance_extensions,
                             extra_device_extensions) )
    {
        fprintf(stderr, C_RED "[vk] Failed to initialize vulkan.\n" C_RESET);
        return nullptr;
    }

    platform->vk_system = vk_system;

    VkCommandPool command_pool;
    {
        VkCommandPoolCreateInfo info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        info.queueFamilyIndex = vk_system.graphics_family;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vkCreateCommandPool(vk_system.device, &info, nullptr, &command_pool);
    }
    VkCommandBuffer command_buffer;
    {
        VkCommandBufferAllocateInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        info.commandPool = command_pool;
        info.commandBufferCount = 1;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        vkAllocateCommandBuffers(vk_system.device, &info, &command_buffer);
    }
    platform->command_pool = command_pool;
    platform->command_buffer = command_buffer;

    m_active = platform.get();
    return platform;
}


void Platform_GLFWVulkanWindow::enter_loop()
{
    double display_time = glfwGetTime();
    while( !glfwWindowShouldClose(glfw_window) )
    {
        glfwPollEvents();
        // Set display time.
        double display_deltatime;
        {
            double new_display_time = glfwGetTime();
            if (new_display_time == display_time) display_deltatime = 0.01;
            else display_deltatime = new_display_time - display_time;
            display_time = new_display_time;
        }

        VkSemaphore acquire_semaphore;
        {
            VkSemaphoreCreateInfo info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            VK_SUCCEED( vkCreateSemaphore(vk_system.device, &info, nullptr, &acquire_semaphore) );
        }
        VkSemaphore release_semaphore;
        {
            VkSemaphoreCreateInfo info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            VK_SUCCEED( vkCreateSemaphore(vk_system.device, &info, nullptr, &release_semaphore) );
        }

        uint32_t image_index;
        VK_SUCCEED( vkAcquireNextImageKHR(vk_system.device, vk_system.swap_chain, ~0ull, acquire_semaphore, VK_NULL_HANDLE, &image_index) );
        assert( image_index < vk_system.swap_chain_num_images );

        vkResetCommandPool(vk_system.device, command_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT );

        {
            VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            VK_SUCCEED( vkBeginCommandBuffer(command_buffer, &begin_info) );

            VkClearColorValue color = { 0,0,0,1 };
            VkImageSubresourceRange range = {};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.levelCount = 1;
            range.layerCount = 1;
            vkCmdClearColorImage(command_buffer, vk_system.swap_chain_images[image_index], VK_IMAGE_LAYOUT_GENERAL, &color, 1, &range);

            VK_SUCCEED( vkEndCommandBuffer(command_buffer) );
        }

        VkPipelineStageFlags stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &acquire_semaphore;
        submit_info.pWaitDstStageMask = &stage;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &release_semaphore;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;
        vkQueueSubmit(vk_system.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);

        DisplayRefreshEvent e;
        e.time = display_time;
        e.dt = display_deltatime;
        int framebuffer_width, framebuffer_height;
        glfwGetFramebufferSize(glfw_window, &framebuffer_width, &framebuffer_height);
        e.framebuffer.width = (uint16_t) framebuffer_width;
        e.framebuffer.height = (uint16_t) framebuffer_height;
        emit_display_refresh_event(e);

        VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &release_semaphore; //?
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &vk_system.swap_chain;
        present_info.pImageIndices = &image_index;
        VK_SUCCEED( vkQueuePresentKHR(vk_system.graphics_queue, &present_info) );

        VK_SUCCEED( vkDeviceWaitIdle(vk_system.device) );

        vkDestroySemaphore(vk_system.device, acquire_semaphore, nullptr);
        vkDestroySemaphore(vk_system.device, release_semaphore, nullptr);
    }

    glfwDestroyWindow(glfw_window);
    glfwTerminate();
}
