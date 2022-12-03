#ifndef VK_UTIL_H
#define VK_UTIL_H

#include <utility>
#include <functional>
#include <vector>

/*
 * Perfect-forwarding trick to wrap vulkan functions whose last two arguments are
 * a pointer to a uint32_t and an array of some vulkan struct.
 * These are intended to be called twice, so this wrapper helps.
 *
 * Example:
 * std::vector<VkQueueFamilyProperties> queue_family_properties =
 *     vk_get_vector<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, vk_physical_device);
 *
 * Notes:
 *     Why is template deduction failing if the return type is specified by the value being assigned to?
 *     Possibly the compiler could not deduce this.
 */
template <typename T, typename F, typename... Args>
std::vector<T> vk_get_vector(F vulkan_function, Args&&... args)
{
    uint32_t num;
    vulkan_function(std::forward<Args>(args)..., &num, nullptr);
    std::vector<T> items( num );
    vulkan_function(std::forward<Args>(args)..., &num, &items[0]);
    return items;
}


#endif // VK_UTIL_H
