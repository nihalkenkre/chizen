#pragma once

#include "scene.hpp"

#include <string>
#include <vector>

#define CGLM_FORCE_ZERO_TO_ONE
#include <cglm/include/cglm/cglm.h>

class world_scene : public scene
{
public:
    world_scene(const std::string& file_path, renderer* r);

    void handle_mouse_move(const POINT mouse_pos) override;
    void handle_mouse_l_btn_down() override;
    void handle_mouse_l_btn_up() override;
    void handle_mouse_m_btn_down() override;
    void handle_mouse_m_btn_up() override;
    void handle_mouse_r_btn_down() override;
    void handle_mouse_r_btn_repeat() override;
    void handle_mouse_r_btn_up() override;
    void handle_w_down() override;
    void handle_a_down() override;
    void handle_s_down() override;
    void handle_d_down() override;
    void handle_q_down() override;
    void handle_e_down() override;
    void handle_w_up() override;
    void handle_a_up() override;
    void handle_s_up() override;
    void handle_d_up() override;
    void handle_q_up() override;
    void handle_e_up() override;
    void process_input_state() override;
    void update() override;
    void render(renderer* r) override;
    ~world_scene();

private:
    struct camera
    {
        mat4 xform;

        mat4 p;

        vec3 eye;
        vec3 dir;
        vec3 right;
        vec3 up;
    };

    void import_scene_data(const cgltf_data* data);
    input_state i = {};
    camera cam = {};
    RECT wnd_rect = {};
};