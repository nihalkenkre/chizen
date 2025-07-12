#pragma once

#include "../common/camera.h"
#include "mesh_optix.h"

typedef struct scene_optix
{
    mesh_optix* meshes;
    size_t meshes_count;

    camera camera;

    OptixTraversableHandle scn_hnd;
} scene_optix;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
    scene_optix scene_optix_create(const char* gltf_path, const OptixDeviceContext ctx, const cudaStream_t stream);
    void scene_optix_destroy(scene_optix s);
#ifdef __cplusplus
}
#endif // __cplusplus

