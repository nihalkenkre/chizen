#include "ray.h"

ray ray_create(vec3 org, vec3 dir)
{
    ray r = {
        .org = {org[0], org[1], org[2]},
        .dir = {dir[0], dir[1], dir[2]},
        .inv_dir = { 1.f / r.dir[0], 1.f / r.dir[1], 1.f / r.dir[2]},
        .sign = {r.inv_dir[0] < 0.f, r.inv_dir[1] < 0.f, r.inv_dir[2] < 0.f},
    };

    return r;
}

void ray_at(ray r, const float t, vec3 pt)
{
    glm_vec3_scale(r.dir, t, pt);
    glm_vec3_add(r.org, pt, pt);
}
