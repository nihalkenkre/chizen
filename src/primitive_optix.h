#pragma once

#include "bbox.h"
#include "material.h"
#include "triangle.h"

#include <optix.h>
#include <cuda_runtime.h>
#include <cgltf/cgltf.h>

typedef struct primitive_optix
{
    triangle* tris;
    size_t tris_count;

    material material;
    bbox bbox;

    OptixTraversableHandle gas_hnd;
    CUdeviceptr vertex_buffer;
} primitive_optix;

primitive_optix primitive_optix_create(cgltf_primitive* curr_prim, mat4 node_xform, const OptixDeviceContext ctx, const cudaStream_t stream);
void primitive_optix_destroy(primitive_optix p);
