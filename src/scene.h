#pragma once

#include "camera.h"
#include "bbox.h"

typedef struct primitive
{
    vec3* positions;
    size_t positions_count;

    vec3* normals;
    size_t normals_count;

    vec2* uvs;
    size_t uvs_count;

    bbox bbox;
} primitive;

typedef struct mesh
{
    primitive* prims;
    size_t prims_count;
    char *name;
} mesh;

typedef struct scene 
{
    mesh* meshes;
    size_t meshes_count;

    camera camera;
} scene;

scene scene_parse_gltf(const char* gltf_path);
void scene_destroy(scene s);
