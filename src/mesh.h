#pragma once

#include <cgltf/cgltf.h>
#include <cglm/include/cglm/cglm.h>
#include "primitive.h"

typedef struct mesh
{
    mat4 xform;
    primitive* prims;
    size_t prims_count;
} mesh;

mesh mesh_create(cgltf_node* node);
void mesh_destroy(mesh m);