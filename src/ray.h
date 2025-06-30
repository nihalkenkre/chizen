#pragma once

#include <cglm/include/cglm/cglm.h>

typedef struct ray
{
    vec3 org;
    vec3 dir;
    ivec3 sign;
    vec3 inv_dir;
} ray;

ray ray_create(vec3 org, vec3 dir);
void ray_at(const ray r, const float t, vec3 pt);
