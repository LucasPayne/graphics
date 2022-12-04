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

#include "ansi_color.h"

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

bool CreateVulkanSystem(VulkanSystem *vk_system,
                        const std::vector<std::string> &extra_explicit_layers,
                        const std::vector<std::string> &extra_instance_extensions,
                        const std::vector<std::string> &extra_device_extensions)
{
    /*
     * Create the vulkan instance and return imported associated vulkan handles
     * in the VulkanSystem struct.
     *
     * Specification of created vulkan system:
     *     One instance.
     *     One logical device preferably created for a discrete GPU.
     *     This device must support presentation.
     *     This device must expose compute and graphics capabilities.
     *     One graphics capable queue.
     *     One compute capable queue. (Note: The queues may coincide).
     */
    std::set<std::string> _explicit_layers = {
    };
    std::set<std::string> _instance_extensions = {
        "VK_KHR_surface"
    };
    std::set<std::string> _device_extensions = {
    };
    #define create_string_vector(NAME)\
        for (auto &str : extra_##NAME) _##NAME .insert(str);\
        std::vector<const char *> NAME;\
        NAME.reserve(_##NAME.size());\
        for (auto &str : _##NAME) NAME.push_back(str.c_str());
    create_string_vector(explicit_layers);
    create_string_vector(instance_extensions);
    create_string_vector(device_extensions);

    VkInstance vk_instance;
    {
        VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.apiVersion = VK_API_VERSION_1_3;

        // Check availability of instance extensions.
        std::vector<VkExtensionProperties> instance_extension_properties =
            vk_get_vector<VkExtensionProperties>(vkEnumerateInstanceExtensionProperties, nullptr);
        for (const char *requested_extension_name : instance_extensions)
        {
            bool has_extension = false;
            for (auto &ext : instance_extension_properties)
            {
                if ( strcmp(requested_extension_name, ext.extensionName) == 0 )
                    has_extension = true;
            }
            if ( !has_extension )
            {
                fprintf(stderr, C_RED "[%s] Requested instance extension \"%s\" not available.\n" C_RESET, __func__, requested_extension_name);
                return false;
            }
        }

        // Check availability of requested layers.
        std::vector<VkLayerProperties> layer_properties =
            vk_get_vector<VkLayerProperties>(vkEnumerateInstanceLayerProperties);
        for (const char *requested_layer_name : explicit_layers)
        {
            bool has_layer = false;
            for (auto &layer : layer_properties)
            {
                if ( strcmp(requested_layer_name, layer.layerName) == 0 )
                    has_layer = true;
            }
            if ( !has_layer )
            {
                fprintf(stderr, C_RED "[%s] Requested explicit layer \"%s\" not available.\n" C_RESET, __func__, requested_layer_name);
                return false;
            }
        }

        printf(C_CYAN "Available instance extensions (%zu):\n" C_RESET, instance_extension_properties.size());
        for (auto &ext : instance_extension_properties)
        {
            printf("    %s (spec %u)\n",
                   ext.extensionName,
                   ext.specVersion);
        }
        printf(C_CYAN "Using explicit layers:\n" C_RESET);
        for (const char *layer : explicit_layers)
        {
            printf("    %s\n", layer);
        }
        printf(C_CYAN "Using instance extensions:\n" C_RESET);
        for (const char *extension : instance_extensions)
        {
            printf("    %s\n", extension);
        }

        VkInstanceCreateInfo info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        info.pApplicationInfo = &app_info;
        info.enabledLayerCount = explicit_layers.size();
        info.ppEnabledLayerNames = &explicit_layers[0];
        info.enabledExtensionCount = instance_extensions.size();
        info.ppEnabledExtensionNames = &instance_extensions[0];
        VK_SUCCEED(vkCreateInstance(&info, nullptr, &vk_instance));
    }

    /*
     * Create a vulkan physical device.
     */
    VkPhysicalDevice vk_physical_device;
    {
        std::vector<VkPhysicalDevice> physical_devices =
            vk_get_vector<VkPhysicalDevice>(vkEnumeratePhysicalDevices, vk_instance);
        if ( physical_devices.size() == 0 ) return false;

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

        printf(C_CYAN "%zu physical devices:\n" C_RESET, physical_devices.size());
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
        std::vector<VkExtensionProperties> device_extension_properties =
            vk_get_vector<VkExtensionProperties>(vkEnumerateDeviceExtensionProperties, vk_physical_device, nullptr);
        printf(C_CYAN "Available device extensions (%zu):\n" C_RESET, device_extension_properties.size());
        #if 0
        for (auto &ext : device_extension_properties)
        {
            printf("    %s (spec %u)\n",
                   ext.extensionName,
                   ext.specVersion);
        }
        #else
        printf("    ...\n");
        #endif

        // Check availability of device extensions.
        for (const char *requested_extension_name : device_extensions)
        {
            bool has_extension = false;
            for (auto &ext : device_extension_properties)
            {
                if ( strcmp(requested_extension_name, ext.extensionName) == 0 )
                    has_extension = true;
            }
            if ( !has_extension )
            {
                fprintf(stderr, C_RED "[%s] Requested device extension \"%s\" not available.\n" C_RESET, __func__, requested_extension_name);
                return false;
            }
        }

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
        if ( vk_device_graphics_family == UINT32_MAX )
        {
            fprintf(stderr, C_RED "[%s] Chosen device has no graphics-capable queue family." C_RESET, __func__);
            return false;
        }
        if ( vk_device_compute_family == UINT32_MAX )
        {
            fprintf(stderr, C_RED "[%s] Chosen device has no compute-capable queue family." C_RESET, __func__);
            return false;
        }

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
        info.enabledExtensionCount = device_extensions.size();
        info.ppEnabledExtensionNames = &device_extensions[0];
        VK_SUCCEED(vkCreateDevice(vk_physical_device, &info, nullptr, &vk_device));
    }

    /*
     * Query the opaque queue handles.
     */
    VkQueue vk_device_graphics_queue;
    vkGetDeviceQueue(vk_device, vk_device_graphics_family, 0, &vk_device_graphics_queue);
    VkQueue vk_device_compute_queue;
    vkGetDeviceQueue(vk_device, vk_device_compute_family, 0, &vk_device_compute_queue);

    /*
     * Set up the VulkanSystem.
     */
    vk_system->instance = vk_instance;
    vk_system->physical_device = vk_physical_device;
    vk_system->device = vk_device;
    vk_system->device_graphics_family = vk_device_graphics_family;
    vk_system->device_compute_family = vk_device_compute_family;
    vk_system->device_graphics_queue = vk_device_graphics_queue;
    vk_system->device_compute_queue = vk_device_compute_queue;
    return true;
}

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
    
    /*
     * Create the VulkanSystem.
     */
    std::vector<std::string> extra_layers = {
    };
    std::vector<std::string> extra_instance_extensions = {
    };
    std::vector<std::string> extra_device_extensions = {
    };
    // Add glfw's required extensions (e.g. KHR_surface).
    {
        uint32_t num_glfw_instance_extensions;
        const char **glfw_instance_extensions = glfwGetRequiredInstanceExtensions( &num_glfw_instance_extensions );
        for ( uint32_t i = 0; i < num_glfw_instance_extensions; i++)
            extra_instance_extensions.emplace_back( glfw_instance_extensions[i] );
    }
    VulkanSystem vk_system;
    if ( !CreateVulkanSystem(&vk_system,
                             extra_layers,
                             extra_instance_extensions,
                             extra_device_extensions) )
    {
        fprintf(stderr, C_RED "[vk] Failed to initialize vulkan.\n" C_RESET);
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *glfw_window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
    if (glfw_window == nullptr)
    {
        fprintf(stderr, C_RED "[GLFW] Failed to create window.\n" C_RESET);
        exit(EXIT_FAILURE);
    }

    while(!glfwWindowShouldClose(glfw_window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(glfw_window);
    glfwTerminate();

    return 0;
}
