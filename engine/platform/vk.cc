#include "vk.h"
#include "vk_util.h"
#include "vk_print.h"
#include "ansi_color.h"
#include <set>

bool CreateVulkanSystem(VulkanSystem *vk_system,
                        std::function<bool(VkInstance, VkPhysicalDevice, VulkanSystemCreateSurfaceOutput *)> create_surface,
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
     *         Acquired images can be used as VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT and VK_IMAGE_USAGE_TRANSFER_DST_BIT.
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
    #undef create_string_vector

    VkInstance vk_instance;
    {
        VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        //app_info.apiVersion = VK_API_VERSION_1_3;
        app_info.apiVersion = VK_API_VERSION_1_2;

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
        printf(C_CYAN "Available layers:\n" C_RESET);
        for (auto &layer : layer_properties )
        {
            printf("    %s\n", layer.layerName);
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
     * Create a vulkan surface.
     */
    VkSurfaceKHR vk_surface;
    VulkanSystemCreateSurfaceOutput created_surface;
    {
        if ( !create_surface(vk_instance, vk_physical_device, &created_surface) )
        {
            fprintf(stderr, C_RED "[%s] Failed to create vulkan surface.", __func__);
            return false;
        }
        vk_surface = created_surface.surface;
        // Determine if the surface has the necessary capabilities.
        // If not, fail to create the VulkanSystem.
        VkSurfaceCapabilitiesKHR vk_surface_capabilities;
        VK_SUCCEED( vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, vk_surface, &vk_surface_capabilities) );

        if ( created_surface.initial_framebuffer_pixel_width < vk_surface_capabilities.minImageExtent.width ||
             created_surface.initial_framebuffer_pixel_width > vk_surface_capabilities.maxImageExtent.width ||
             created_surface.initial_framebuffer_pixel_height < vk_surface_capabilities.maxImageExtent.height ||
             created_surface.initial_framebuffer_pixel_height > vk_surface_capabilities.maxImageExtent.height )
        {
            fprintf(stderr, C_RED "[%s] The requested initial framebuffer size is not supported by the vulkan surface.\n", __func__);
            return false;
        }
    }

    /*
     * Create a vulkan logical device.
     */
    VkDevice vk_device;
    uint32_t vk_graphics_family;
    uint32_t vk_compute_family;
    uint32_t vk_presentation_family;
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

        vk_graphics_family = UINT32_MAX;
        vk_compute_family = UINT32_MAX;
        vk_presentation_family = UINT32_MAX;
        for (uint32_t i = 0; i < queue_families.size(); i++)
        {
            auto &family = queue_families[i];
            if ( vk_graphics_family == UINT32_MAX && family.queueFlags & VK_QUEUE_GRAPHICS_BIT )
            {
                vk_graphics_family = i;
            }
            if ( vk_compute_family == UINT32_MAX && family.queueFlags & VK_QUEUE_COMPUTE_BIT )
            {
                vk_compute_family = i;
            }
            // The VK_KHR_surface extension exposes an equivalent capability test.
            VkBool32 presentation_supported;
            VK_SUCCEED( vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device, i, vk_surface, &presentation_supported ) );
            if ( presentation_supported )
            {
                vk_presentation_family = i;
            }
        }
        if ( vk_graphics_family == UINT32_MAX )
        {
            fprintf(stderr, C_RED "[%s] Chosen device has no graphics-capable queue family." C_RESET, __func__);
            return false;
        }
        if ( vk_compute_family == UINT32_MAX )
        {
            fprintf(stderr, C_RED "[%s] Chosen device has no compute-capable queue family." C_RESET, __func__);
            return false;
        }
        if ( vk_presentation_family == UINT32_MAX )
        {
            fprintf(stderr, C_RED "[%s] Chosen device has no presentation-capable queue family." C_RESET, __func__);
            return false;
        }
        std::set<uint32_t> used_queue_families = {
            vk_graphics_family,
            vk_compute_family,
            vk_presentation_family
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
     * Create a vulkan swap chain.
     */
    VkSwapchainKHR vk_swap_chain;
    VkFormat vk_swap_chain_image_format = VK_FORMAT_B8G8R8A8_SRGB;
    VkColorSpaceKHR vk_swap_chain_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkPresentModeKHR vk_swap_chain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    {
        VkSurfaceCapabilitiesKHR vk_surface_capabilities;
        VK_SUCCEED( vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, vk_surface, &vk_surface_capabilities) );

        auto vk_surface_formats =
            vk_get_vector<VkSurfaceFormatKHR>(vkGetPhysicalDeviceSurfaceFormatsKHR, vk_physical_device, vk_surface);
        if ( vk_surface_formats.empty() )
        {
            fprintf(stderr, C_RED "[%s] The created vulkan surface does not support at least one image format.", __func__);
            return false;
        }
        if ( !std::any_of( vk_surface_formats.begin(),
                          vk_surface_formats.end(),
                          [&](auto v) { return v.format == vk_swap_chain_image_format && v.colorSpace == vk_swap_chain_color_space; } ) )
        {
            fprintf(stderr, C_RED "[%s] The created vulkan surface does not support the required image format and color space combination.", __func__);
            return false;
        }
        auto vk_surface_present_modes =
            vk_get_vector<VkPresentModeKHR>(vkGetPhysicalDeviceSurfacePresentModesKHR, vk_physical_device, vk_surface);
        // Note: The FIFO present mode is guaranteed by the Vulkan spec.
        if ( vk_surface_present_modes.empty() )
        {
            fprintf(stderr, C_RED "[%s] The created vulkan surface does not support at least one present mode.", __func__);
            return false;
        }

        uint32_t vk_swap_chain_image_count;
        {
            uint32_t min_image_count = vk_surface_capabilities.minImageCount;
            uint32_t max_image_count = vk_surface_capabilities.maxImageCount == 0 ? UINT32_MAX : vk_surface_capabilities.maxImageCount;
            vk_swap_chain_image_count = std::max(min_image_count, 1u);
            // Try to use more than one image, if possible.
            if ( vk_swap_chain_image_count == 1 && max_image_count > 1 )
            {
                vk_swap_chain_image_count += 1;
            }
            // The VulkanSystem struct sets a cap on the number of images.
            vk_swap_chain_image_count = std::min(vk_swap_chain_image_count, VULKAN_SYSTEM_SWAP_CHAIN_MAX_NUM_IMAGES);
            assert( vk_swap_chain_image_count <= max_image_count );
        }

        VkSwapchainCreateInfoKHR info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        info.surface = vk_surface;
        info.imageFormat = vk_swap_chain_image_format;
        info.imageColorSpace = vk_swap_chain_color_space;
        info.presentMode = vk_swap_chain_present_mode;
        info.imageExtent.width = created_surface.initial_framebuffer_pixel_width;
        info.imageExtent.height = created_surface.initial_framebuffer_pixel_height;
        info.minImageCount = vk_swap_chain_image_count;
        info.imageArrayLayers = 1;
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.preTransform = vk_surface_capabilities.currentTransform;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.clipped = VK_TRUE;
        info.oldSwapchain = VK_NULL_HANDLE;
        VK_SUCCEED( vkCreateSwapchainKHR(vk_device, &info, nullptr, &vk_swap_chain ) );
    }

    /*
     * Query the opaque queue handles.
     */
    VkQueue vk_graphics_queue;
    vkGetDeviceQueue(vk_device, vk_graphics_family, 0, &vk_graphics_queue);
    VkQueue vk_compute_queue;
    vkGetDeviceQueue(vk_device, vk_compute_family, 0, &vk_compute_queue);
    VkQueue vk_presentation_queue;
    vkGetDeviceQueue(vk_device, vk_presentation_family, 0, &vk_presentation_queue);

    /*
     * Query the images from the swap chain.
     */
    auto vk_swap_chain_images = 
        vk_get_vector<VkImage>(vkGetSwapchainImagesKHR, vk_device, vk_swap_chain);
    assert( vk_swap_chain_images.size() <= VULKAN_SYSTEM_SWAP_CHAIN_MAX_NUM_IMAGES );

    /*
     * Set up color target image views for each image in the swap chain.
     */
    std::vector<VkImageView> vk_swap_chain_color_target_image_views;
    for (uint32_t i = 0; i < vk_swap_chain_images.size(); i++)
    {
        VkImageViewCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        info.image = vk_swap_chain_images[i];
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = vk_swap_chain_image_format;
        info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;
        VkImageView view;
        VK_SUCCEED( vkCreateImageView(vk_device, &info, nullptr, &view ) );
        vk_swap_chain_color_target_image_views.push_back(view);
    }

    /*
     * Set up the VulkanSystem.
     */
    vk_system->instance = vk_instance;
    vk_system->physical_device = vk_physical_device;
    vk_system->device = vk_device;
    vk_system->graphics_family = vk_graphics_family;
    vk_system->compute_family = vk_compute_family;
    vk_system->presentation_family = vk_presentation_family;
    vk_system->graphics_queue = vk_graphics_queue;
    vk_system->compute_queue = vk_compute_queue;
    vk_system->presentation_queue = vk_presentation_queue;
    vk_system->surface = vk_surface;
    vk_system->swap_chain = vk_swap_chain;
    vk_system->swap_chain_num_images = vk_swap_chain_images.size();
    for (uint32_t i = 0; i < vk_swap_chain_images.size(); i++)
    {
        vk_system->swap_chain_images[i] = vk_swap_chain_images[i];
        vk_system->swap_chain_color_target_image_views[i] = vk_swap_chain_color_target_image_views[i];
    }
    return true;
}
