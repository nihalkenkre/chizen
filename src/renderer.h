#pragma once

#include <Windows.h>
#include <stdint.h>

#define CGLM_FORCE_ZERO_TO_ONE
#include <cglm/include/cglm/cglm.h>

typedef struct renderer
{
    void (*import_scene_data)(const char* file_path);
    void (*resize)(const uint32_t width, const uint32_t height);
    void (*begin_frame)();
    void (*clear_frame)(const float color[]);
    void (*render_world)(const mat4 cam_xform);
    void (*end_frame)();
    void (*clear_scene_data)();
    void (*render_offline)(const uint32_t render_width, const uint32_t render_height, uint8_t* pixels);
    void (*shutdown)();

    RECT wnd_rect;
} renderer;