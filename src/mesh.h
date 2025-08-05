#pragma once

#include "primitive.h"
#include <cglm/include/cglm/cglm.h>

typedef struct mesh
{
    primitive* prims;
    size_t prims_count;
} mesh;

mesh mesh_create(const cgltf_data* data, cgltf_mesh* curr_mesh, const OptixDeviceContext ctx, const cudaStream_t stream);
void mesh_destroy(mesh m);