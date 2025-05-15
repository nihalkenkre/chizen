#pragma once

#include <Windows.h>

#include "renderer.hpp"

#include <string>
#include <memory>

struct input_state
{
    POINT curr_mouse_pos;
    POINT last_mouse_pos;

    bool l_btn_down;
    bool m_btn_down;
    bool r_btn_down;

    bool w_down;
    bool a_down;
    bool s_down;
    bool d_down;
    bool q_down;
    bool e_down;
};

class scene
{
public:
    scene() {}

    virtual void handle_mouse_move(const POINT mouse_pos) = 0;
    virtual void handle_mouse_l_btn_down() = 0;
    virtual void handle_mouse_l_btn_up() = 0;
    virtual void handle_mouse_m_btn_down() = 0;
    virtual void handle_mouse_m_btn_up() = 0;
    virtual void handle_mouse_r_btn_down() = 0;
    virtual void handle_mouse_r_btn_repeat() = 0;
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
    virtual void process_input_state() = 0;
    virtual void update() = 0;
    virtual void render(renderer* r) = 0;
    virtual ~scene() {}

protected:
    input_state i = {};
};