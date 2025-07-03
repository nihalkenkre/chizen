#include "camera.h"
#include "utils.h"

camera camera_create(cgltf_node* camera_node)
{
    camera c = { 0 };

    mat4 xform = { 0 };
    get_xform_matrix_for_node(camera_node, xform);

    vec3 t = { 0 }; mat4 r = { 0 }; vec3 s = { 0 };
    glm_decompose(xform, t, r, s);

    glm_vec3_copy(t, c.pos);
    c.fov = camera_node->camera->data.perspective.yfov;
    c.znear = camera_node->camera->data.perspective.znear;
    c.zfar = camera_node->camera->data.perspective.has_zfar ? camera_node->camera->data.perspective.has_zfar : 1000.f;
    c.aspect_ratio = camera_node->camera->data.perspective.has_aspect_ratio ? camera_node->camera->data.perspective.has_aspect_ratio : 16.f / 9.f;

    glm_vec3_rotate_m4(r, GLM_ZUP, c.w);
    glm_vec3_rotate_m4(r, GLM_YUP, c.v);
    glm_vec3_rotate_m4(r, GLM_XUP, c.u);

    return c;
}

void camera_destroy(camera c)
{
}
