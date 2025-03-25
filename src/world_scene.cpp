#include "world_scene.hpp"
#include "dx12_renderer.hpp"
#include "vulkan_renderer.hpp"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

world_scene::world_scene(const std::string &path, renderer *r)
{
    std::cout << __FUNCTION__ << " " << path << '\n';
    cgltf_options options = {};
    cgltf_data *data = nullptr;

    if (!(cgltf_parse_file(&options, path.c_str(), &data) == cgltf_result_success && cgltf_validate(data) == cgltf_result_success && cgltf_load_buffers(&options, data, path.c_str()) == cgltf_result_success))
    {
        std::cerr << "ERR Could not parse gltf file\n";
    }
    else
    {
        this->import_scene(data);
        r->import_scene(data);
    }
}

void world_scene::import_scene(const cgltf_data *data)
{
}

void world_scene::handle_mouse_move(const POINT mouse_pos)
{
    this->mouse_pos = mouse_pos;
}

void world_scene::update()
{
}

void world_scene::render(renderer *r)
{
}

world_scene::~world_scene()
{
    std::cout << __FUNCTION__ << '\n';
}