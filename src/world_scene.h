#pragma once

#include "scene.h"
#include "renderer.h"

void world_scene_init(const char* file_path, scene* s, renderer* r);
void world_scene_handle_mouse_move(const POINT mouse_pos);
void world_scene_handle_mouse_l_btn_down();
void world_scene_handle_mouse_l_btn_up();
void world_scene_handle_mouse_m_btn_down();
void world_scene_handle_mouse_m_btn_up();
void world_scene_handle_mouse_r_btn_down();
void world_scene_handle_mouse_r_btn_repeat();
void world_scene_handle_mouse_r_btn_up();
void world_scene_handle_w_down();
void world_scene_handle_a_down();
void world_scene_handle_s_down();
void world_scene_handle_d_down();
void world_scene_handle_q_down();
void world_scene_handle_e_down();
void world_scene_handle_w_up();
void world_scene_handle_a_up();
void world_scene_handle_s_up();
void world_scene_handle_d_up();
void world_scene_handle_q_up();
void world_scene_handle_e_up();
void world_scene_process_input_state();
void world_scene_update();
void world_scene_render(renderer *r);
void world_scene_shutdown();