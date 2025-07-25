#pragma once

#include "primitive.h"
#include <cglm/include/cglm/cglm.h>

typedef struct mesh
{
    mat4 xform;
    primitive* prims;
    size_t prims_count;

    // 1 OptixInstance per primitive.
    OptixInstance* instances;
} mesh;

mesh mesh_create(const cgltf_data* data, cgltf_node* curr_node, const OptixDeviceContext ctx, const cudaStream_t stream);
void mesh_destroy(mesh m);