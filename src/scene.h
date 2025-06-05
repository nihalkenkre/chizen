#pragma once

#include <windef.h>


typedef struct scene
{
    void (*handle_mouse_move)(const POINT mouse_pos);
    void (*handle_mouse_l_btn_down)();
    void (*handle_mouse_l_btn_up)();
    void (*handle_mouse_m_btn_down)();
    void (*handle_mouse_m_btn_up)();
    void (*handle_mouse_r_btn_down)();
    void (*handle_mouse_r_btn_repeat)();
    void (*handle_mouse_r_btn_up)();
    void (*handle_w_down)();
    void (*handle_a_down)();
    void (*handle_s_down)();
    void (*handle_d_down)();
    void (*handle_q_down)();
    void (*handle_e_down)();
    void (*handle_w_up)();
    void (*handle_a_up)();
    void (*handle_s_up)();
    void (*handle_d_up)();
    void (*handle_q_up)();
    void (*handle_e_up)();
    void (*process_input_state)();
    void (*update)();
    //void (*render)(renderer* r);
} scene;