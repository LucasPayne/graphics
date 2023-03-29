#include "renderer/renderer.h"
#include <stdio.h>
#include <assert.h>

void Renderer::render(int x, int y, int width, int height)
{
    printf("rendering...\n");
}

void Renderer::set_api(VulkanSystem *_vk)
{
    assert(graphics_api == GraphicsAPI::None);
    graphics_api = GraphicsAPI::Vulkan;
    vk = _vk;
}

