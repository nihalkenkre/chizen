#pragma once

#include "primitive_optix.h"
#include <cglm/include/cglm/cglm.h>

typedef struct mesh_optix
{
    mat4 xform;
    primitive_optix* prims;
    size_t prims_count;

    OptixTraversableHandle ias_hnd;
} mesh_optix;

mesh_optix mesh_optix_create(cgltf_node* curr_node, const OptixDeviceContext ctx, const cudaStream_t stream);
void mesh_optix_destroy(mesh_optix m);