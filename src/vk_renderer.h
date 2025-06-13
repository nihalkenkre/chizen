#pragma once

#include "renderer.h"

void vk_renderer_init(renderer* r, const HWND h_wnd);
void vk_renderer_import_scene_data(const cgltf_data* data);
void vk_renderer_resize(const uint32_t width, const uint32_t height);
void vk_renderer_begin_frame(void);
void vk_renderer_clear_frame(const float color[]);
void vk_renderer_render_world(const mat4 cam_xform);
void vk_renderer_end_frame(void);
void vk_renderer_render_offline(const uint32_t render_width, const uint32_t render_height, uint8_t* pixels);
void vk_renderer_clear_scene_data(void);
void vk_renderer_shutdown(void);
