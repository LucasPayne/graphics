#include <GLFW/glfw3.h>
#ifndef GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#endif

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define VK_SUCCEED(call) \
    do { \
        VkResult result = call; \
        assert(result == VK_SUCCESS); \
    } while (0)

int main()
{
    if (!glfwInit())
    {
        std::cerr << "[GLFW] Failed to initialize glfw.\n";
        exit(EXIT_FAILURE);
    }

    if (!glfwVulkanSupported())
    {
        std::cerr << "[GLFW] Vulkan is not supported (Is the loader installed correctly?)\n";
    }


    VkInstance vk_instance;
    {
        VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.apiVersion = VK_VERSION_1_3;

        VkInstanceCreateInfo info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        info.pApplicationInfo = &app_info;
        info.flags = 0;
        info.enabledLayerCount = ;
        info.ppEnabledLayerNames = ;
        info.enabledExtensionCount = ;
        info.ppEnabledExtensionNames = ;
        VK_SUCCEED(vkCreateInstance(&info, nullptr, &vk_instance));
    }


    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* glfw_window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::cout << extensionCount << " extensions supported\n";

    if (glfw_window == nullptr) {
        std::cerr << "[GLFW] Failed to create window.\n";
        return -1;
    }

    while(!glfwWindowShouldClose(glfw_window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(glfw_window);
    glfwTerminate();
    return 0;
}
