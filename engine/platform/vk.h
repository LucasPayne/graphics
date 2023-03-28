#ifndef VK_H_
#define VK_H_
#include <vulkan/vulkan.h>
#include <functional>
#include <string>

struct VulkanSystem
{
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;

    // Queue family index to create queues of a certain capability from.
    // NOTE: Might not need to save this, if not going to make more queues.
    //     X-- Need this for creating command pools.
    uint32_t graphics_family;
    uint32_t compute_family;
    uint32_t presentation_family;

    // A single queue is made available for each capability. These queues might be the same.
    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueue presentation_queue;

    VkSurfaceKHR surface;
    VkSwapchainKHR swap_chain;

    uint32_t swap_chain_num_images;
    #define VULKAN_SYSTEM_SWAP_CHAIN_MAX_NUM_IMAGES 4u
    VkImage swap_chain_images[VULKAN_SYSTEM_SWAP_CHAIN_MAX_NUM_IMAGES];
    VkImageView swap_chain_color_target_image_views[VULKAN_SYSTEM_SWAP_CHAIN_MAX_NUM_IMAGES];
};

struct VulkanSystemCreateSurfaceOutput
{
    VkSurfaceKHR surface;
    uint32_t initial_framebuffer_pixel_width;
    uint32_t initial_framebuffer_pixel_height;
};

bool CreateVulkanSystem(VulkanSystem *vk_system,
                        std::function<bool(VkInstance, VkPhysicalDevice, VulkanSystemCreateSurfaceOutput *)> create_surface,
                        const std::vector<std::string> &extra_explicit_layers,
                        const std::vector<std::string> &extra_instance_extensions,
                        const std::vector<std::string> &extra_device_extensions);

// Helper macro
#define VK_SUCCEED(call) \
    do { \
        VkResult result = call; \
        assert(result == VK_SUCCESS); \
    } while (0)

#endif // VK_H_
