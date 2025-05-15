#pragma once

#include "scene.hpp"
#include "renderer.hpp"

#include <string>
#include <iostream>

class default_scene : public scene
{
public:
    void handle_mouse_move(const POINT mouse_pos) override;
    void handle_mouse_l_btn_down() override {}
    void handle_mouse_l_btn_up() override {}
    void handle_mouse_m_btn_down() override {}
    void handle_mouse_m_btn_up() override {}
    void handle_mouse_r_btn_down() override {}
    void handle_mouse_r_btn_repeat() override {}
    void handle_mouse_r_btn_up() override {}
    void handle_w_down() override {}
    void handle_a_down() override {}
    void handle_s_down() override {}
    void handle_d_down() override {}
    void handle_q_down() override {}
    void handle_e_down() override {}
    void handle_w_up() override {}
    void handle_a_up() override {}
    void handle_s_up() override {}
    void handle_d_up() override {}
    void handle_q_up() override {}
    void handle_e_up() override {}
    void process_input_state() override {}
    void update() override;
    void render(renderer* r) override;
    ~default_scene();
};