#pragma once

#include "camera.h"
#include "mesh.h"

typedef struct scene
{
    mesh* meshes;
    size_t meshes_count;

    camera camera;
} scene;

scene scene_parse_gltf(const char* gltf_path);
void scene_destroy(scene s);
