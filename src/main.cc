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
// #define UINT8_MAX std::numeric_limits<uint8_t>::max()
// #define UINT16_MAX std::numeric_limits<uint16_t>::max()
// #define UINT32_MAX std::numeric_limits<uint32_t>::max()
// #define UINT64_MAX std::numeric_limits<uint64_t>::max()
#include <optional>

#include "vk_util.h"
#include "vk_print.h"

#include <json/json.h>

#define assert_msg(expr, message) {\
    bool val = expr;\
    if (!val)\
    {\
        fprintf(stderr, "assert failed: " #expr);\
        fprintf(stderr, message);\
        exit(EXIT_FAILURE);\
    }\
}

#define VK_SUCCEED(call) \
    do { \
        VkResult result = call; \
        assert(result == VK_SUCCESS); \
    } while (0)


struct VulkanSystem
{
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;

    // Queue family index to create queues of a certain capability from.
    // NOTE: Might not need to save this, if not going to make more queues.
    uint32_t device_graphics_family;
    uint32_t device_compute_family;

    // A single queue is made available for each capability. These queues might be the same.
    VkQueue device_graphics_queue;
    VkQueue device_compute_queue;
};

int main()
{
    if ( !glfwInit() )
    {
        std::cerr << "[GLFW] Failed to initialize glfw.\n";
        exit(EXIT_FAILURE);
    }

    if ( !glfwVulkanSupported() )
    {
        std::cerr << "[GLFW] Vulkan is not supported (Is the loader installed correctly?)\n";
    }

    /*
     * Create the vulkan instance.
     */
    VkInstance vk_instance;
    {
        VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.apiVersion = VK_API_VERSION_1_3;

        std::vector<const char *> explicit_layers = {
            "VK_LAYER_KHRONOS_validation"
        };
        std::vector<const char *> extensions = {
            //
        };
        // Add glfw's required extensions (e.g. KHR_surface).
        uint32_t num_glfw_extensions;
        const char **glfw_extensions = glfwGetRequiredInstanceExtensions( &num_glfw_extensions );
        for ( uint32_t i = 0; i < num_glfw_extensions; i++) extensions.push_back( glfw_extensions[i] );

        std::vector<VkExtensionProperties> extension_properties =
            vk_get_vector<VkExtensionProperties>(vkEnumerateInstanceExtensionProperties, nullptr);
        for (auto &ext : extension_properties)
        {
            printf("    %s (spec %u)\n",
                   ext.extensionName,
                   ext.specVersion);
        }
        
        VkInstanceCreateInfo info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        info.pApplicationInfo = &app_info;
        info.enabledLayerCount = explicit_layers.size();
        info.ppEnabledLayerNames = &explicit_layers[0];
        info.enabledExtensionCount = extensions.size();
        info.ppEnabledExtensionNames = &extensions[0];
        VK_SUCCEED(vkCreateInstance(&info, nullptr, &vk_instance));
    }

    /*
     * Create a vulkan physical device.
     */
    VkPhysicalDevice vk_physical_device;
    {
        std::vector<VkPhysicalDevice> physical_devices =
            vk_get_vector<VkPhysicalDevice>(vkEnumeratePhysicalDevices, vk_instance);
        assert(physical_devices.size() > 0);

        // Pick a physical device.
        // Prefer a discrete gpu, but otherwise just use the first device.
        uint32_t selected_device_index = 0;
        vk_physical_device = physical_devices[0];
        for (uint32_t i = 0; i < physical_devices.size(); i++)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                vk_physical_device = physical_devices[i];
                selected_device_index = i;
            }
        }

        printf("%zu physical devices:\n", physical_devices.size());
        for (uint32_t i = 0; i < physical_devices.size(); i++)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
            printf("    %s%s, %s\n",
                   i == selected_device_index ? "(selected) " : "",
                   properties.deviceName,
                   vk_enum_to_string_VkPhysicalDeviceType(properties.deviceType));
        }
    }

    /*
     * Create a vulkan logical device.
     */
    VkDevice vk_device;
    uint32_t vk_device_graphics_family;
    uint32_t vk_device_compute_family;
    {
        std::vector<VkExtensionProperties> device_extensions =
            vk_get_vector<VkExtensionProperties>(vkEnumerateDeviceExtensionProperties, vk_physical_device, nullptr);
        printf("%zu device extensions supported:\n", device_extensions.size());
        #if 0
        for (auto &ext : device_extensions)
        {
            printf("    %s (spec %u)\n",
                   ext.extensionName,
                   ext.specVersion);
        }
        #endif

        std::vector<VkQueueFamilyProperties> queue_families =
            vk_get_vector<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, vk_physical_device);

        for (uint32_t i = 0; i < queue_families.size(); i++)
        {
            auto &family = queue_families[i];
            Json::Value json;
            int idx = 0;
            if ( family.queueFlags & VK_QUEUE_GRAPHICS_BIT )
                json["queueFlags"][idx++] = "VK_QUEUE_GRAPHICS_BIT";
            if ( family.queueFlags & VK_QUEUE_COMPUTE_BIT )
                json["queueFlags"][idx++] = "VK_QUEUE_COMPUTE_BIT";
            if ( family.queueFlags & VK_QUEUE_TRANSFER_BIT )
                json["queueFlags"][idx++] = "VK_QUEUE_TRANSFER_BIT";
            if ( family.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT )
                json["queueFlags"][idx++] = "VK_QUEUE_SPARSE_BINDING_BIT";
            if ( family.queueFlags & VK_QUEUE_PROTECTED_BIT )
                json["queueFlags"][idx++] = "VK_QUEUE_PROTECTED_BIT";
            json["queueCount"] = family.queueCount;
            printf("%u: ", i);
            Json::cout << json << "\n";
        }

        vk_device_graphics_family = UINT32_MAX;
        vk_device_compute_family = UINT32_MAX;
        for (uint32_t i = 0; i < queue_families.size(); i++)
        {
            auto &family = queue_families[i];
            if ( vk_device_graphics_family == UINT32_MAX && family.queueFlags & VK_QUEUE_GRAPHICS_BIT )
            {
                vk_device_graphics_family = i;
            }
            if ( vk_device_compute_family == UINT32_MAX && family.queueFlags & VK_QUEUE_COMPUTE_BIT )
            {
                vk_device_compute_family = i;
            }
        }
        assert_msg(vk_device_graphics_family != UINT32_MAX, "[vk] Chosen device has no graphics-capable queue family.");
        assert_msg(vk_device_compute_family != UINT32_MAX, "[vk] Chosen device has no compute-capable queue family.");

        std::vector<VkDeviceQueueCreateInfo> queue_infos;
        {
            VkDeviceQueueCreateInfo info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            info.queueFamilyIndex = vk_device_graphics_family;
            info.queueCount = 1;
            float queue_priority = 1.0f;
            info.pQueuePriorities = &queue_priority;
            queue_infos.push_back(info);
        }
        {
            // If possible, share the graphics and compute queue.
            if ( vk_device_graphics_family != vk_device_compute_family )
            {
                VkDeviceQueueCreateInfo info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
                info.queueFamilyIndex = vk_device_compute_family;
                info.queueCount = 1;
                float queue_priority = 1.0f;
                info.pQueuePriorities = &queue_priority;
                queue_infos.push_back(info);
            }
        }
        VkDeviceCreateInfo info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        info.queueCreateInfoCount = queue_infos.size();
        info.pQueueCreateInfos = &queue_infos[0];
        VK_SUCCEED(vkCreateDevice(vk_physical_device, &info, nullptr, &vk_device));
    }

    /*
     * Query the opaque queue handles.
     */
    VkQueue vk_device_graphics_queue;
    vkGetDeviceQueue(vk_device, vk_device_graphics_family, 0, &vk_device_graphics_queue);
    VkQueue vk_device_compute_queue;
    vkGetDeviceQueue(vk_device, vk_device_compute_family, 0, &vk_device_compute_queue);

    VulkanSystem vk_system;
    vk_system.instance = vk_instance;
    vk_system.physical_device = vk_physical_device;
    vk_system.device = vk_device;
    vk_system.device_graphics_family = vk_device_graphics_family;
    vk_system.device_compute_family = vk_device_compute_family;
    vk_system.device_graphics_queue = vk_device_graphics_queue;
    vk_system.device_compute_queue = vk_device_compute_queue;


    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *glfw_window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
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
