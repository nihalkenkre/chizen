#pragma once

#include <Windows.h>

#include <cstdint>

namespace optx_renderer
{
    void render(const uint32_t render_width, const uint32_t render_height, uint8_t * pixels);
}