#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap/VkBootstrap.h>

#include <stdlib.h>
#include <iostream>

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

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";


    vkb::InstanceBuilder instance_builder;
    instance_builder.request_validation_layers();
    instance_builder.use_default_debug_messenger();
    auto instance_builder_ret = instance_builder 
        .set_app_name ("Application")
        .require_api_version(1,2,0)
        .build();

    if (!instance_builder_ret) {
        std::cerr << "[VkBootstrap] Failed to create Vulkan instance. Error: " << instance_builder_ret.error().message() << "\n";
        return -1;
    } 
    vkb::Instance vkb_instance = instance_builder_ret.value();

    auto system_info_ret = vkb::SystemInfo::get_system_info();
    if (!system_info_ret) {
        std::cerr << "[VkBootstrap] Failed to query Vulkan system info: " << system_info_ret.error().message() << "\n";
        return -1;
    }
    auto system_info = system_info_ret.value();
    if (system_info.is_layer_available("VK_LAYER_LUNARG_api_dump")) {
        instance_builder.enable_layer("VK_LAYER_LUNARG_api_dump");
    }
    if (system_info.validation_layers_available) {
        instance_builder.enable_validation_layers();
    }

    VkSurfaceKHR surface = nullptr;
    VkResult err = glfwCreateWindowSurface(vkb_instance.instance, window, NULL, &surface);
    if (err != VK_SUCCESS) {
        std::cerr << "[GLFW] Failed to create Vulkan window surface.\n";
        return -1;
    }

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    vkb::destroy_instance(vkb_instance);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
