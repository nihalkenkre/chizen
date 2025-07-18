#pragma once

#include "camera.h"
#include "mesh.h"

typedef struct scene
{
    mesh* meshes;
    size_t meshes_count;

    camera camera;

    OptixTraversableHandle ias_hnd;
    CUdeviceptr d_ias_op_buffer;
} scene;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
    scene scene_create(const char* gltf_path, const OptixDeviceContext ctx, const cudaStream_t stream);
    void scene_destroy(scene s);
#ifdef __cplusplus
}
#endif // __cplusplus

