#define GLFW_INCLUDE_VULKAN

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
    uint32_t device_presentation_family;

    // A single queue is made available for each capability. These queues might be the same.
    VkQueue device_graphics_queue;
    VkQueue device_compute_queue;
    VkQueue device_presentation_queue;

    VkSurfaceKHR surface;
};

bool CreateVulkanSystem(VulkanSystem *vk_system,
                        std::function<bool(VkInstance, VkPhysicalDevice, VkSurfaceKHR *)> create_surface,
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
     *     One compute capable queue.
     *     One presentation capable queue.
     *     (Note: The queues may coincide).
     *     One surface.
     *         This surface is created by a passed function which only has access to the VkInstance and VkPhysicalDevice.
     *         It is the responsibility of the passed create_surface function to provide a surface with sufficient capabilities.
     *         This surface must have at least one image format and at least one present mode.
     *         The surface must support the format VK_FORMAT_B8G8R8A8_SRGB.
     *         The surface must support the color space VK_COLOR_SPACE_SRGB_NONLINEAR_KHR.
     *     One swapchain.
     *         Uses image format VK_FORMAT_B8G8R8A8_SRGB.
     *         Uses color space VK_COLOR_SPACE_SRGB_NONLINEAR_KHR.
     *         Uses present mode VK_PRESENT_MODE_FIFO_KHR.
     */
    std::set<std::string> _explicit_layers = {
    };
    std::set<std::string> _instance_extensions = {
        "VK_KHR_surface"
    };
    std::set<std::string> _device_extensions = {
        "VK_KHR_swapchain"
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
        auto instance_extension_properties =
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
        auto layer_properties =
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
        auto physical_devices =
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
     * Create a vulkan surface and swap chain.
     */
    VkSurfaceKHR vk_surface;
    VkFormat vk_swapchain_image_format = VK_FORMAT_B8G8R8A8_SRGB;
    VkColorSpaceKHR vk_swapchain_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkPresentModeKHR vk_swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    {
        if ( !create_surface(vk_instance, vk_physical_device, &vk_surface) )
        {
            fprintf(stderr, C_RED "[%s] Failed to create vulkan surface.", __func__);
            return false;
        }
        // Determine if the surface has the necessary capabilities.
        // If not, fail to create the VulkanSystem.
        VkSurfaceCapabilitiesKHR vk_surface_capabilities;
        VK_SUCCEED( vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, vk_surface, &vk_surface_capabilities) );
        auto vk_surface_formats =
            vk_get_vector<VkSurfaceFormatKHR>(vkGetPhysicalDeviceSurfaceFormatsKHR, vk_physical_device, vk_surface);
        if ( vk_surface_formats.empty() )
        {
            fprintf(stderr, C_RED "[%s] The created vulkan surface does not support at least one image format.", __func__);
            return false;
        }
        if ( std::find( vk_surface_formats.begin(),
                        vk_surface_formats.end(),
                        { vk_swapchain_image_format, vk_swapchain_color_space } ) == vk_surface_formats.end() )
        {
            fprintf(stderr, C_RED "[%s] The created vulkan surface does not support the required image format and color space combination.", __func__);
            return false;
        }

        // Display the surface formats.
        {
            Json::Value json;
            for (uint32_t i = 0; i < vk_surface_formats.size(); i++)
            {
                json[0]["format"] = vk_enum_to_string_VkFormat(vk_surface_formats[i].format);
            }
            Json::cout << json << "\n";
        }

        auto vk_surface_present_modes =
            vk_get_vector<VkPresentModeKHR>(vkGetPhysicalDeviceSurfacePresentModesKHR, vk_physical_device, vk_surface);
        if ( vk_surface_present_modes.empty() )
        {
            fprintf(stderr, C_RED "[%s] The created vulkan surface does not support at least one present mode.", __func__);
            return false;
        }
        // Note: The FIFO present mode is guaranteed by the Vulkan spec.
    }

    /*
     * Create a vulkan logical device.
     */
    VkDevice vk_device;
    uint32_t vk_device_graphics_family;
    uint32_t vk_device_compute_family;
    uint32_t vk_device_presentation_family;
    {
        auto device_extension_properties =
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

        auto queue_families =
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
        vk_device_presentation_family = UINT32_MAX;
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
            // The VK_KHR_surface extension exposes an equivalent capability test.
            VkBool32 presentation_supported;
            VK_SUCCEED( vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device, i, vk_surface, &presentation_supported ) );
            if ( presentation_supported )
            {
                vk_device_presentation_family = i;
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
        if ( vk_device_presentation_family == UINT32_MAX )
        {
            fprintf(stderr, C_RED "[%s] Chosen device has no presentation-capable queue family." C_RESET, __func__);
            return false;
        }
        std::set<uint32_t> used_queue_families = {
            vk_device_graphics_family,
            vk_device_compute_family,
            vk_device_presentation_family
        };
        std::vector<VkDeviceQueueCreateInfo> queue_infos;
        for (uint32_t family : used_queue_families)
        {
            VkDeviceQueueCreateInfo info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            info.queueFamilyIndex = family;
            info.queueCount = 1;
            float queue_priority = 1.0f;
            info.pQueuePriorities = &queue_priority;
            queue_infos.push_back(info);
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
    VkQueue vk_device_presentation_queue;
    vkGetDeviceQueue(vk_device, vk_device_presentation_family, 0, &vk_device_presentation_queue);

    /*
     * Set up the VulkanSystem.
     */
    vk_system->instance = vk_instance;
    vk_system->physical_device = vk_physical_device;
    vk_system->device = vk_device;
    vk_system->device_graphics_family = vk_device_graphics_family;
    vk_system->device_compute_family = vk_device_compute_family;
    vk_system->device_presentation_family = vk_device_presentation_family;
    vk_system->device_graphics_queue = vk_device_graphics_queue;
    vk_system->device_compute_queue = vk_device_compute_queue;
    vk_system->device_presentation_queue = vk_device_presentation_queue;
    vk_system->surface = vk_surface;
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
    
    ///*
    // * Create the VulkanSystem.
    // */
    //VkSurfaceKHR surface;
    //VkResult err = glfwCreateWindowSurface(vk_instance, window, NULL, &surface);
    //if (err != VK_SUCCESS)
    //{
    //    fprintf(stderr, C_RED  "[GLFW] Vulkan is not supported (Is the loader installed correctly?)\n" C_RESET);
    //    "[GLFW] Failed to create Vulkan window surface.\n";
    //    return -1;
    //}

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *glfw_window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
    if (glfw_window == nullptr)
    {
        fprintf(stderr, C_RED "[GLFW] Failed to create window.\n" C_RESET);
        exit(EXIT_FAILURE);
    }

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

    auto create_surface = [glfw_window](VkInstance vk_instance, VkPhysicalDevice vk_physical_device, VkSurfaceKHR *vk_surface_ptr)->bool
    {
        VkResult err = glfwCreateWindowSurface(vk_instance, glfw_window, NULL, vk_surface_ptr);
        return err == VK_SUCCESS;
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

    while(!glfwWindowShouldClose(glfw_window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(glfw_window);
    glfwTerminate();

    return 0;
}
