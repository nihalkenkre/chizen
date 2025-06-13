#include "world_scene.h"
#include <cgltf/cgltf.h>

#define CGLM_FORCE_ZERO_TO_ONE
#include <cglm/include/cglm/cglm.h>

typedef struct camera
{
    mat4 xform;

    mat4 p;

    vec3 eye;
    vec3 dir;
    vec3 right;
    vec3 up;
} camera;

float CAMERA_MOVEMENT_SPEED = 0.2f;
float CAMERA_LOOK_AROUND_SCALE = 0.2f;

input_state ws_is = { 0 };

camera cam = {
    .eye = { 10, 10, 10 },
};

RECT wnd_rect = { 0 };

void world_scene_init(const char* file_path, scene* s, renderer* r)
{
    cgltf_options options = { 0 };
    cgltf_data* data = NULL;

    if (cgltf_parse_file(&options, file_path, &data) != cgltf_result_success ||
        cgltf_validate(data) != cgltf_result_success ||
        cgltf_load_buffers(&options, data, file_path) != cgltf_result_success)
    {
        printf("ERR Could not parse gltf file\n");
    }
    else
    {
        r->clear_scene_data();
        r->import_scene_data(data);
        cgltf_free(data);
    }

    // initial viewport camera xform
    // get direction vector
    vec3 center = { 0,0,0 };
    glm_vec3_sub(center, cam.eye, cam.dir);
    glm_vec3_normalize(cam.dir);

    // get right vector
    vec3 up = { 0, 1, 0 };
    glm_vec3_cross(up, cam.dir, cam.right);

    // get up vector
    glm_normalize(cam.right);
    glm_cross(cam.dir, cam.right, cam.up);

    // xform mat4
    glm_look(cam.eye, cam.dir, up, cam.xform);

    // proj mat4
    glm_perspective(glm_rad(60.f), (float)(r->wnd_rect.right - r->wnd_rect.left) / (float)(r->wnd_rect.bottom - r->wnd_rect.top), 0.1f, 50.f, cam.p);
    cam.p[1][1] *= -1;

    // v * p
    glm_mul(cam.p, cam.xform, cam.xform);

    s->handle_mouse_move = world_scene_handle_mouse_move;
    s->handle_mouse_l_btn_down = world_scene_handle_mouse_l_btn_down;
    s->handle_mouse_l_btn_up = world_scene_handle_mouse_l_btn_up;
    s->handle_mouse_m_btn_down = world_scene_handle_mouse_m_btn_down;
    s->handle_mouse_m_btn_up = world_scene_handle_mouse_m_btn_up;
    s->handle_mouse_r_btn_down = world_scene_handle_mouse_r_btn_down;
    s->handle_mouse_r_btn_repeat = world_scene_handle_mouse_r_btn_repeat;
    s->handle_mouse_r_btn_up = world_scene_handle_mouse_r_btn_up;
    s->handle_w_down = world_scene_handle_w_down;
    s->handle_a_down = world_scene_handle_a_down;
    s->handle_s_down = world_scene_handle_s_down;
    s->handle_d_down = world_scene_handle_d_down;
    s->handle_q_down = world_scene_handle_q_down;
    s->handle_e_down = world_scene_handle_e_down;
    s->handle_w_up = world_scene_handle_w_up;
    s->handle_a_up = world_scene_handle_a_up;
    s->handle_s_up = world_scene_handle_s_up;
    s->handle_d_up = world_scene_handle_d_up;
    s->handle_q_up = world_scene_handle_q_up;
    s->handle_e_up = world_scene_handle_e_up;
    s->process_input_state = world_scene_process_input_state;
    s->update = world_scene_update;
    s->render = world_scene_render;
    s->shutdown = world_scene_shutdown;

    wnd_rect = r->wnd_rect;
}

void world_scene_handle_mouse_move(const POINT mouse_pos)
{
    ws_is.curr_mouse_pos = mouse_pos;
}

void world_scene_handle_mouse_l_btn_down()
{
    ws_is.l_btn_down = true;
}

void world_scene_handle_mouse_l_btn_up()
{
    ws_is.l_btn_down = false;
}

void world_scene_handle_mouse_m_btn_down()
{
    ws_is.m_btn_down = true;
}

void world_scene_handle_mouse_m_btn_up()
{
    ws_is.m_btn_down = false;
}

void world_scene_handle_mouse_r_btn_down()
{
    ws_is.r_btn_down = true;
    ws_is.last_mouse_pos = ws_is.curr_mouse_pos;
}

void world_scene_handle_mouse_r_btn_repeat()
{
    vec3 up = { 0, 1, 0 };

    // yaw
    float yaw_step = (float)(GLM_PI * 2) / (float)(wnd_rect.right - wnd_rect.left);
    float yaw = (float)(ws_is.last_mouse_pos.x - ws_is.curr_mouse_pos.x) * yaw_step;

    glm_vec3_rotate(cam.dir, yaw * CAMERA_LOOK_AROUND_SCALE, up);
    glm_vec3_rotate(cam.right, yaw * CAMERA_LOOK_AROUND_SCALE, up);

    // pitch
    float pitch_step = (float)(GLM_PI) / (float)(wnd_rect.bottom - wnd_rect.top);
    float pitch = (float)(ws_is.curr_mouse_pos.y - ws_is.last_mouse_pos.y) * pitch_step;
    glm_vec3_rotate(cam.dir, pitch * CAMERA_LOOK_AROUND_SCALE, cam.right);
    glm_vec3_rotate(cam.up, pitch * CAMERA_LOOK_AROUND_SCALE, cam.right);

    glm_look(cam.eye, cam.dir, up, cam.xform);
    glm_mul(cam.p, cam.xform, cam.xform);

    ws_is.last_mouse_pos = ws_is.curr_mouse_pos;
}

void world_scene_handle_mouse_r_btn_up()
{
    ws_is.r_btn_down = false;
}

void world_scene_handle_w_down()
{
    ws_is.w_down = true;

    vec3 translate = { CAMERA_MOVEMENT_SPEED,  CAMERA_MOVEMENT_SPEED, CAMERA_MOVEMENT_SPEED };
    glm_vec3_mul(translate, cam.dir, translate);
    glm_vec3_add(cam.eye, translate, cam.eye);
    vec3 up = { 0,1,0 };
    glm_look(cam.eye, cam.dir, up, cam.xform);
    glm_mul(cam.p, cam.xform, cam.xform);
}

void world_scene_handle_a_down()
{
    ws_is.a_down = true;

    vec3 translate = { CAMERA_MOVEMENT_SPEED ,  CAMERA_MOVEMENT_SPEED, CAMERA_MOVEMENT_SPEED };
    glm_vec3_mul(translate, cam.right, translate);
    glm_vec3_add(cam.eye, translate, cam.eye);
    vec3 up = { 0,1,0 };
    glm_look(cam.eye, cam.dir, up, cam.xform);
    glm_mul(cam.p, cam.xform, cam.xform);
}

void world_scene_handle_s_down()
{
    ws_is.s_down = true;

    vec3 translate = { -CAMERA_MOVEMENT_SPEED,  -CAMERA_MOVEMENT_SPEED, -CAMERA_MOVEMENT_SPEED };
    glm_vec3_mul(translate, cam.dir, translate);
    glm_vec3_add(cam.eye, translate, cam.eye);
    vec3 up = { 0,1,0 };
    glm_look(cam.eye, cam.dir, up, cam.xform);
    glm_mul(cam.p, cam.xform, cam.xform);
}

void world_scene_handle_d_down()
{
    ws_is.d_down = true;

    vec3 up = { 0,1,0 };
    vec3 translate = { -CAMERA_MOVEMENT_SPEED,  -CAMERA_MOVEMENT_SPEED, -CAMERA_MOVEMENT_SPEED };
    glm_vec3_mul(translate, cam.right, translate);
    glm_vec3_add(cam.eye, translate, cam.eye);
    glm_look(cam.eye, cam.dir, up, cam.xform);
    glm_mul(cam.p, cam.xform, cam.xform);
}

void world_scene_handle_q_down()
{
    ws_is.q_down = true;

    vec3 up = { 0,1,0 };
    vec3 translate = { CAMERA_MOVEMENT_SPEED,  CAMERA_MOVEMENT_SPEED, CAMERA_MOVEMENT_SPEED };
    glm_vec3_mul(translate, up, translate);
    glm_vec3_add(cam.eye, translate, cam.eye);
    glm_look(cam.eye, cam.dir, up, cam.xform);
    glm_mul(cam.p, cam.xform, cam.xform);
}

void world_scene_handle_e_down()
{
    ws_is.e_down = true;

    vec3 up = { 0,1,0 };
    vec3 translate = { -CAMERA_MOVEMENT_SPEED,  -CAMERA_MOVEMENT_SPEED, -CAMERA_MOVEMENT_SPEED };
    glm_vec3_mul(translate, up, translate);
    glm_vec3_add(cam.eye, translate, cam.eye);
    glm_look(cam.eye, cam.dir, up, cam.xform);
    glm_mul(cam.p, cam.xform, cam.xform);
}

void world_scene_handle_w_up()
{
    ws_is.w_down = false;
}

void world_scene_handle_a_up()
{
    ws_is.a_down = false;
}

void world_scene_handle_s_up()
{
    ws_is.s_down = false;
}

void world_scene_handle_d_up()
{
    ws_is.d_down = false;
}

void world_scene_handle_q_up()
{
    ws_is.q_down = false;
}

void world_scene_handle_e_up()
{
    ws_is.e_down = false;
}

void world_scene_process_input_state()
{
}

void world_scene_update()
{
}

void world_scene_render(renderer* r)
{
    r->begin_frame();
    float color[] = { 0.2f, 0.2f, 0.2f, 1.f };
    r->clear_frame(color);
    r->render_world(cam.xform);
    r->end_frame();
}

void world_scene_shutdown()
{
}
