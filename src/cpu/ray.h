#pragma once

#include <cglm/include/cglm/cglm.h>

typedef struct ray
{
    vec3 org;
    vec3 dir;
    vec3 inv_dir;
    uint8_t sign[3];
} ray;

ray ray_create(vec3 org, vec3 dir);
void ray_at(const ray r, const float t, vec3 pt);
