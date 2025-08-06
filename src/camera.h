#pragma once

#include <cglm/include/cglm/cglm.h>
#include <cgltf/cgltf.h>
#include "error.h"

typedef struct camera
{
    vec3 pos;
    float fov;
    float znear;
    float zfar;
    float aspect_ratio;
    vec3 w;
    vec3 v;
    vec3 u;
    CHIZEN_RESULT result;
} camera;

camera camera_create_from_gltf(cgltf_node* camera_node);
void camera_destroy(camera c);
