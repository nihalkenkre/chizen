#pragma once

#include "camera.h"
#include "mesh.h"
#include "image.h"
#include "texture.h"
#include "material.h"

typedef struct scene
{
    mesh* meshes;
    size_t meshes_count;

    camera camera;

    image* d_images;
    texture* d_textures;
    material* d_materials;

    OptixTraversableHandle ias_hnd;
    CUdeviceptr d_ias_op_buffer;
} scene;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
    scene scene_create_from_gltf(cgltf_data* gltf_data, const OptixDeviceContext ctx, const cudaStream_t stream);
    void scene_destroy(scene s);
#ifdef __cplusplus
}
#endif // __cplusplus

