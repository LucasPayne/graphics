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

RenderEntity Renderer::get_camera()
{
    return (RenderEntityType::Camera << 32) & 0;
}

void Renderer::set_transform(RenderEntity entity, Transform transform)
{
    RenderEntityType type = entity >> 32;
    uint32_t index = entity & ((1 << 32)-1);
    switch (type)
    {
    case RenderEntityType::Camera:
        //-Dirty camera.
        camera_transform = transform;
        break;
    case RenderEntityType::PolygonMesh:
        //-Dirty acceleration structures.
        assert(index >= 0 && index < polygon_mesh_transforms.size());
        polygon_mesh_transforms[index] = transform;
        break;
    case RenderEntityType::PointLight:
        assert(index >= 0 && index < point_light_transforms.size());
        point_light_transforms[index] = transform;
        break;
    }
}
