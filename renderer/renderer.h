#ifndef RENDERER_H_
#define RENDERER_H_
#include "engine/platform/platform.h"
#include "engine/platform/vk.h"
#include <vector>

typedef uint64_t RenderEntity;
enum class RenderEntityType
{
    Camera,
    PolygonMesh,
    PointLight
};

struct PolygonMeshCreateInfo
{
    int num_vertices;
    vec3 *positions;
    vec3 *normals;
    int num_indices;
    uint16_t *indices;
}

class Renderer
{
public:
    void render(int x, int y, int width, int height);
    void set_api(VulkanSystem *_vk);

    RenderEntity create_polygon_mesh(RenderPolygonCreateInfo info);
    RenderEntity create_point_light();
    RenderEntity get_camera();

    RenderTransform create_transform(vec3 position, vec3 euler_angles);
    void set_transform(RenderEntity entity, RenderTransform transform);

    void destroy_entity(RenderEntity entity);
    void set_attribute(RenderEntity entity, AttributeType attribute, vec4 value);

private:
    VulkanSystem *vk;

    Camera camera;
    Transform camera_transform;
    std::vector<PolygonMesh> polygon_meshes;
    std::vector<Transform> polygon_mesh_transforms;
    std::vector<PointLight> point_lights;
    std::vector<Transform> point_light_transforms;
};

#endif // RENDERER_H_
