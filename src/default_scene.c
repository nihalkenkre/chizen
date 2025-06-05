#include "default_scene.h"

typedef struct input_state
{
    POINT curr_mouse_pos;
    POINT last_mouse_pos;
} input_state;

input_state i = {0};

void default_scene_init(scene* s)
{
    s->handle_mouse_move = default_scene_handle_mouse_move;
    s->handle_mouse_l_btn_down = default_scene_handle_mouse_l_btn_down;
    s->handle_mouse_l_btn_up = default_scene_handle_mouse_l_btn_up;
    s->handle_mouse_m_btn_down = default_scene_handle_mouse_m_btn_down;
    s->handle_mouse_m_btn_up = default_scene_handle_mouse_m_btn_up;
    s->handle_mouse_r_btn_down = default_scene_handle_mouse_r_btn_down;
    s->handle_mouse_r_btn_repeat = default_scene_handle_mouse_r_btn_repeat;
    s->handle_mouse_r_btn_up = default_scene_handle_mouse_r_btn_up;
    s->handle_w_down = default_scene_handle_w_down;
    s->handle_a_down = default_scene_handle_a_down;
    s->handle_s_down = default_scene_handle_s_down;
    s->handle_d_down = default_scene_handle_d_down;
    s->handle_q_down = default_scene_handle_q_down;
    s->handle_e_down = default_scene_handle_e_down;
    s->handle_w_up = default_scene_handle_w_up;
    s->handle_a_up = default_scene_handle_a_up;
    s->handle_s_up = default_scene_handle_s_up;
    s->handle_d_up = default_scene_handle_d_up;
    s->handle_q_up = default_scene_handle_q_up;
    s->handle_e_up = default_scene_handle_e_up;
    s->process_input_state = default_scene_process_input_state;
    s->update = default_scene_update;
    s->render = default_scene_render;
    s->shutdown = default_scene_shutdown;
}

void default_scene_handle_mouse_move(const POINT mouse_pos)
{
    i.curr_mouse_pos = mouse_pos;
}

void default_scene_handle_mouse_l_btn_down(void)
{
}

void default_scene_handle_mouse_m_btn_down(void)
{
}

void default_scene_handle_mouse_l_btn_up(void)
{
}

void default_scene_handle_mouse_m_btn_up(void)
{
}

void default_scene_handle_mouse_r_btn_down(void)
{
}

void default_scene_handle_mouse_r_btn_repeat(void)
{
}

void default_scene_handle_mouse_r_btn_up(void)
{
}

void default_scene_handle_w_down(void)
{
}

void default_scene_handle_a_down(void)
{
}

void default_scene_handle_s_down(void)
{
}

void default_scene_handle_d_down(void)
{
}

void default_scene_handle_q_down(void)
{
}

void default_scene_handle_e_down(void)
{
}

void default_scene_handle_w_up(void)
{
}

void default_scene_handle_a_up(void)
{
}

void default_scene_handle_s_up(void)
{
}

void default_scene_handle_d_up(void)
{
}

void default_scene_handle_q_up(void)
{
}

void default_scene_handle_e_up(void)
{
}

void default_scene_process_input_state(void)
{
}

void default_scene_update(void)
{
}

void default_scene_render(renderer* r)
{
    r->begin_frame();
    float color[] = {
        (float)i.curr_mouse_pos.x / (r->wnd_rect.right - r->wnd_rect.left),
        0.0,
        (float)i.curr_mouse_pos.y / (r->wnd_rect.bottom - r->wnd_rect.top),
        1.0,
    };
    r->clear_frame(color);
    r->end_frame();
}

void default_scene_shutdown(void)
{
}
