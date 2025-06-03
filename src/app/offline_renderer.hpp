#pragma once

#include <cstdint>
#include "vk_objects.hpp"
#include "renderer.hpp"

namespace offline_renderer
{
    void render(const size_t render_width, const size_t render_height, const std::string& file_path, const void* cam_xform, uint8_t* pixels);
}