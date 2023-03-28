#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#ifndef GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#endif

#include "vk.h"

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

#include "ansi_color.h"

#include <json/json.h>

int main()
{
    if ( !glfwInit() )
    {
        fprintf(stderr, C_RED  "[GLFW] Failed to initialize glfw.\n" C_RESET);
        exit(EXIT_FAILURE);
    }

    if ( !glfwVulkanSupported() )
    {
        fprintf(stderr, C_RED  "[GLFW] Vulkan is not supported (Is the loader installed correctly?)\n" C_RESET);
        exit(EXIT_FAILURE);
    }

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
        exit(EXIT_FAILURE);
    }
    glfwSetWindowPos(glfw_window, window_x, window_y);

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
        exit(EXIT_FAILURE);
    }

    Engine engine;

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

    while( !glfwWindowShouldClose(glfw_window) )
    {
        glfwPollEvents();

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

            VkClearColorValue color = { 1,0,0,1 };
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
        vkQueueSubmit(vk_system.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);

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

    return 0;
}
