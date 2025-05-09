#pragma once

#include <Windows.h>

#include <cgltf/cgltf.h>
#include <string>

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
    virtual void handle_mouse_move(const POINT mouse_pos) = 0;
    virtual void handle_mouse_l_btn_down() = 0;
    virtual void handle_mouse_l_btn_up() = 0;
    virtual void handle_mouse_m_btn_down() = 0;
    virtual void handle_mouse_m_btn_up() = 0;
    virtual void handle_mouse_r_btn_down() = 0;
    virtual void handle_mouse_r_btn_up() = 0;
    virtual void handle_w_down() = 0;
    virtual void handle_a_down() = 0;
    virtual void handle_s_down() = 0;
    virtual void handle_d_down() = 0;
    virtual void handle_q_down() = 0;
    virtual void handle_e_down() = 0;
    virtual void handle_w_up() = 0;
    virtual void handle_a_up() = 0;
    virtual void handle_s_up() = 0;
    virtual void handle_d_up() = 0;
    virtual void handle_q_up() = 0;
    virtual void handle_e_up() = 0;
    virtual void resize(const uint32_t width, const uint32_t height) = 0;
    virtual void begin_frame() = 0;
    virtual void clear_frame(const float color[]) = 0;
    virtual void render_world() = 0;
    virtual void end_frame() = 0;
    virtual void clear_scene_data() = 0;
    virtual ~renderer() {}

    RECT wnd_rect = { 0,0,0,0 };
};