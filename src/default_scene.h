#pragma once

#include <windows.h>

#include "scene.h"
#include "renderer.h"

void default_scene_init(scene* s);
void default_scene_handle_mouse_move(const POINT mouse_pos);
void default_scene_handle_mouse_l_btn_down(void);
void default_scene_handle_mouse_m_btn_down(void);
void default_scene_handle_mouse_l_btn_up(void);
void default_scene_handle_mouse_m_btn_up(void);
void default_scene_handle_mouse_r_btn_down(void);
void default_scene_handle_mouse_r_btn_repeat(void);
void default_scene_handle_mouse_r_btn_up(void);
void default_scene_handle_w_down(void);
void default_scene_handle_a_down(void);
void default_scene_handle_s_down(void);
void default_scene_handle_d_down(void);
void default_scene_handle_q_down(void);
void default_scene_handle_e_down(void);
void default_scene_handle_w_up(void);
void default_scene_handle_a_up(void);
void default_scene_handle_s_up(void);
void default_scene_handle_d_up(void);
void default_scene_handle_q_up(void);
void default_scene_handle_e_up(void);
void default_scene_process_input_state(void);
void default_scene_update(void);
void default_scene_render(renderer* r);
void default_scene_shutdown(void);
