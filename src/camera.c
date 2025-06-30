#include "camera.h"

#include <string.h>

camera camera_create(const vec3 pos, const vec4 rot, const float fov, const float znear, const float zfar, const float aspect_ratio, const char* name)
{
    camera c = { 0 };
    glm_vec3_copy(pos, c.pos);
    glm_vec3_copy(rot, c.rot);
    c.fov = fov;
    c.znear = znear;
    c.zfar = zfar;
    c.aspect_ratio = aspect_ratio;

    size_t cam_name_len = strlen(name);
    c.name = malloc(cam_name_len + 1);
    strcpy(c.name, name);

    mat4 rot_mat;
    float angle = glm_quat_angle(rot);
    vec3 axis;
    glm_quat_axis(rot, axis);
    glm_rotate_make(rot_mat, angle, axis);

    glm_vec3_rotate_m4(rot_mat, GLM_ZUP, c.w);
    glm_vec3_rotate_m4(rot_mat, GLM_YUP, c.v);
    glm_vec3_rotate_m4(rot_mat, GLM_XUP, c.u);

    return c;
}

void camera_destroy(camera c)
{
    if (c.name != NULL)
        free(c.name);
}
