#ifndef RENDERER_H_
#define RENDERER_H_
#include "engine/platform/platform.h"
#include "engine/platform/vk.h"

enum class GraphicsAPI
{
    None,
    Vulkan,
    OpenGL,
};

class Renderer
{
public:
    void render(int x, int y, int width, int height);
    void set_api(VulkanSystem *_vk);
private:
    GraphicsAPI graphics_api;
    VulkanSystem *vk;
};

#endif // RENDERER_H_
