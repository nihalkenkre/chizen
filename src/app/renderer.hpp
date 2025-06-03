#pragma once

#include <Windows.h>

#include <string>

#define CGLM_FORCE_ZERO_TO_ONE
#include <cglm/include/cglm/cglm.h>

enum RENDERING_API
{
    DX12,
    VULKAN,
};

class renderer
{
public:
    renderer() {}

    virtual void import_scene_data(const std::string& file_path) = 0;
    virtual void resize(const uint32_t width, const uint32_t height) = 0;
    virtual void begin_frame() = 0;
    virtual void clear_frame(const float color[]) = 0;
    virtual void render_world(const mat4 cam_xform) = 0;
    virtual void end_frame() = 0;
    virtual void clear_scene_data() = 0;
    virtual void render_offline(const uint32_t render_width, const uint32_t render_height, uint8_t* pixels) = 0;
    virtual ~renderer() {}

    RECT wnd_rect = { 0,0,0,0 };
    void* scene_data = nullptr;
};
