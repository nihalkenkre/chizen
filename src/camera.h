#pragma once

#include <cglm/include/cglm/cglm.h>

typedef struct camera
{
    vec3 pos;
    vec4 rot;
    float fov;
    float znear;
    float zfar;
    float aspect_ratio;
    vec3 w;
    vec3 v;
    vec3 u;

    char* name;
} camera;

camera camera_create(const vec3 pos, const vec4 rot, const float fov, const float znear, const float zfar, const float aspect_ratio, const char* name);
void camera_destroy(camera c);
