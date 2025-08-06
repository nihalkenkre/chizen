#pragma once

#include "light.h"
#include "camera.h"
#include "mesh.h"
#include "image.h"
#include "texture.h"
#include "material.h"
#include "error.h"

typedef struct scene
{
    camera camera;

    light* d_lights;
    texture* d_textures;
    material* d_materials;

    OptixTraversableHandle ias_hnd;
    RESULT_CODE result;
} scene;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
    scene scene_create_from_gltf(cgltf_data* gltf_data, const OptixDeviceContext ctx, const cudaStream_t stream);
    void scene_destroy(scene s);
#ifdef __cplusplus
}
#endif // __cplusplus

