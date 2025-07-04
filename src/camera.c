#include "camera.h"
#include "utils.h"

camera camera_create(cgltf_node* camera_node)
{
    camera c = {
        .fov = camera_node->camera->data.perspective.yfov,
        .znear = camera_node->camera->data.perspective.znear,
        .zfar = camera_node->camera->data.perspective.has_zfar ? camera_node->camera->data.perspective.has_zfar : 1000.f,
        .aspect_ratio = camera_node->camera->data.perspective.has_aspect_ratio ? camera_node->camera->data.perspective.has_aspect_ratio : 16.f / 9.f,
    };

    mat4 xform = { 0 };
    get_xform_matrix_for_node(camera_node, xform);

    vec4 t = { 0 }; mat4 r = { 0 }; vec3 s = { 0 };
    glm_decompose(xform, t, r, s);

    vec3 t3 = { t[0], t[1], t[2] };
    glm_vec3_copy(t3, c.pos);
    glm_vec3_rotate_m4(r, GLM_XUP, c.u);
    glm_vec3_rotate_m4(r, GLM_YUP, c.v);
    glm_vec3_rotate_m4(r, GLM_ZUP, c.w);

    return c;
}

void camera_destroy(camera c)
{
}
