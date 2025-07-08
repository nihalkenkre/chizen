#pragma once

#include <cglm/include/cglm/cglm.h>

typedef struct triangle
{
    vec3 positions[3];
    vec3 normals[3];
    vec2 uvs[3];
} triangle ;

triangle triangle_create(vec3 positions[], vec3 normals[], vec2 uvs[], uint32_t* indices, size_t i);
