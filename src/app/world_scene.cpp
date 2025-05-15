#include "world_scene.hpp"

#include <cgltf/cgltf.h>
#include <iostream>

constexpr float CAMERA_MOVEMENT_SPEED = 0.20;
constexpr float CAMERA_LOOK_AROUND_SCALE = 0.2;

world_scene::world_scene(const std::string& file_path, renderer* r)
{
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    if (cgltf_parse_file(&options, file_path.c_str(), &data) != cgltf_result_success ||
        cgltf_validate(data) != cgltf_result_success ||
        cgltf_load_buffers(&options, data, file_path.c_str()) != cgltf_result_success)
    {
        std::cerr << "ERR Could not parse gltf file\n";
    }
    else
    {
        this->import_scene_data(data);
        r->clear_scene_data();
        r->import_scene_data(file_path);
        cgltf_free(data);
    }

    // initial viewport camera xform
    // get direction vector
    std::memcpy(cam.eye, vec3{ 10, 10, 10 }, sizeof(vec3));
    glm_vec3_sub(vec3{ 0,0,0 }, cam.eye, cam.dir);
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
    glm_perspective(glm_rad(60.f), static_cast<float>(r->wnd_rect.right - r->wnd_rect.left) / static_cast<float>(r->wnd_rect.bottom - r->wnd_rect.top), 0.1, 50.f, cam.p);
    cam.p[1][1] *= -1;

    // v * p
    glm_mul(cam.p, cam.xform, cam.xform);

    wnd_rect = r->wnd_rect;
}

void world_scene::import_scene_data(const cgltf_data* data)
{
    /*  sd = std::make_unique<scene_data>();

      for (cgltf_size n = 0; n < data->nodes_count; ++n)
      {
          cgltf_node* curr_node = data->nodes + n;

          if (curr_node->mesh == nullptr)
              continue;

          cgltf_mesh* curr_mesh = curr_node->mesh;

          for (cgltf_size p = 0; p < curr_mesh->primitives_count; ++p)
          {
              cgltf_primitive* curr_prim = curr_mesh->primitives + p;

              if (curr_prim->material == nullptr)
              {
                  continue;
              }

              auto mat_it = std::find(sd->mats.begin(), sd->mats.end(), std::string(curr_prim->material->name));

              if (mat_it != sd->mats.end())
              {
                  auto msh_it = std::find(mat_it->meshes.begin(), mat_it->meshes.end(), std::string(curr_prim->material->name));

                  if (msh_it != mat_it->meshes.end())
                  {
                      ++msh_it->prims_count;
                  }
                  else
                  {
                      mesh msh = {};
                      msh.name = std::string(curr_mesh->name);
                      msh.prims_count = 1;

                      mat_it->meshes.push_back(msh);
                  }
              }
              else
              {
                  mesh msh = {};
                  msh.name = std::string(curr_mesh->name);
                  msh.prims_count = 1;

                  material mat = {};
                  mat.name = std::string(curr_prim->material->name);
                  mat.meshes = { msh };

                  sd->mats.push_back(mat);
              }
          }
      }*/
}

void world_scene::handle_mouse_move(const POINT mouse_pos)
{
    i.curr_mouse_pos = mouse_pos;
}

void world_scene::handle_mouse_l_btn_down()
{
    i.l_btn_down = true;
}

void world_scene::handle_mouse_l_btn_up()
{
    i.l_btn_down = false;
}

void world_scene::handle_mouse_m_btn_down()
{
    i.m_btn_down = true;
}

void world_scene::handle_mouse_m_btn_up()
{
    i.m_btn_down = false;
}

void world_scene::handle_mouse_r_btn_down()
{
    i.r_btn_down = true;

    i.last_mouse_pos = i.curr_mouse_pos;
}

void world_scene::handle_mouse_r_btn_repeat()
{
    // yaw
    float yaw_step = static_cast<float>(GLM_PI * 2) / static_cast<float>(wnd_rect.right - wnd_rect.left);
    float yaw = static_cast<float>(i.last_mouse_pos.x - i.curr_mouse_pos.x) * yaw_step;
    glm_vec3_rotate(cam.dir, yaw * CAMERA_LOOK_AROUND_SCALE, vec3{ 0,1,0 });
    glm_vec3_rotate(cam.right, yaw * CAMERA_LOOK_AROUND_SCALE, vec3{ 0,1,0 });

    // pitch
    float pitch_step = static_cast<float>(GLM_PI) / static_cast<float>(wnd_rect.bottom - wnd_rect.top);
    float pitch = static_cast<float>(i.curr_mouse_pos.y - i.last_mouse_pos.y) * pitch_step;
    glm_vec3_rotate(cam.dir, pitch * CAMERA_LOOK_AROUND_SCALE, cam.right);
    glm_vec3_rotate(cam.up, pitch * CAMERA_LOOK_AROUND_SCALE, cam.right);

    glm_look(cam.eye, cam.dir, vec3{ 0,1,0 }, cam.xform);
    glm_mul(cam.p, cam.xform, cam.xform);

    i.last_mouse_pos = i.curr_mouse_pos;
}

void world_scene::handle_mouse_r_btn_up()
{
    i.r_btn_down = false;
}

void world_scene::handle_w_down()
{
    i.w_down = true;

    vec3 translate = { CAMERA_MOVEMENT_SPEED,  CAMERA_MOVEMENT_SPEED, CAMERA_MOVEMENT_SPEED };
    glm_vec3_mul(translate, cam.dir, translate);
    glm_vec3_add(cam.eye, translate, cam.eye);
    glm_look(cam.eye, cam.dir, vec3{ 0,1,0 }, cam.xform);
    glm_mul(cam.p, cam.xform, cam.xform);
}

void world_scene::handle_a_down()
{
    i.a_down = true;

    vec3 translate = { CAMERA_MOVEMENT_SPEED ,  CAMERA_MOVEMENT_SPEED, CAMERA_MOVEMENT_SPEED };
    glm_vec3_mul(translate, cam.right, translate);
    glm_vec3_add(cam.eye, translate, cam.eye);
    glm_look(cam.eye, cam.dir, vec3{ 0,1,0 }, cam.xform);
    glm_mul(cam.p, cam.xform, cam.xform);
}

void world_scene::handle_s_down()
{
    i.s_down = true;

    vec3 translate = { -CAMERA_MOVEMENT_SPEED,  -CAMERA_MOVEMENT_SPEED, -CAMERA_MOVEMENT_SPEED };
    glm_vec3_mul(translate, cam.dir, translate);
    glm_vec3_add(cam.eye, translate, cam.eye);
    glm_look(cam.eye, cam.dir, vec3{ 0,1,0 }, cam.xform);
    glm_mul(cam.p, cam.xform, cam.xform);
}

void world_scene::handle_d_down()
{
    i.d_down = true;

    vec3 translate = { -CAMERA_MOVEMENT_SPEED,  -CAMERA_MOVEMENT_SPEED, -CAMERA_MOVEMENT_SPEED };
    glm_vec3_mul(translate, cam.right, translate);
    glm_vec3_add(cam.eye, translate, cam.eye);
    glm_look(cam.eye, cam.dir, vec3{ 0,1,0 }, cam.xform);
    glm_mul(cam.p, cam.xform, cam.xform);
}

void world_scene::handle_q_down()
{
    i.q_down = true;

    vec3 translate = { CAMERA_MOVEMENT_SPEED,  CAMERA_MOVEMENT_SPEED, CAMERA_MOVEMENT_SPEED };
    glm_vec3_mul(translate, vec3{ 0,1,0 }, translate);
    glm_vec3_add(cam.eye, translate, cam.eye);
    glm_look(cam.eye, cam.dir, vec3{ 0,1,0 }, cam.xform);
    glm_mul(cam.p, cam.xform, cam.xform);
}

void world_scene::handle_e_down()
{
    i.e_down = true;

    vec3 translate = { -CAMERA_MOVEMENT_SPEED,  -CAMERA_MOVEMENT_SPEED, -CAMERA_MOVEMENT_SPEED };
    glm_vec3_mul(translate, vec3{ 0,1,0 }, translate);
    glm_vec3_add(cam.eye, translate, cam.eye);
    glm_look(cam.eye, cam.dir, vec3{ 0,1,0 }, cam.xform);
    glm_mul(cam.p, cam.xform, cam.xform);
}

void world_scene::handle_w_up()
{
    i.w_down = false;
}

void world_scene::handle_a_up()
{
    i.a_down = false;
}

void world_scene::handle_s_up()
{
    i.s_down = false;
}

void world_scene::handle_d_up()
{
    i.d_down = false;
}

void world_scene::handle_q_up()
{
    i.q_down = false;
}

void world_scene::handle_e_up()
{
    i.e_down = false;
}

void world_scene::process_input_state()
{
}

void world_scene::update()
{
}

void world_scene::render(renderer* r)
{
    r->begin_frame();
    float color[] = { 0.2, 0.2, 0.2, 1 };
    r->clear_frame(color);
    r->render_world(cam.xform);
    r->end_frame();
}

world_scene::~world_scene()
{
}