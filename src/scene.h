#pragma once

#include "light.h"
#include "camera.h"
#include "mesh.h"
#include "image.h"
#include "texture.h"
#include "material.h"
#include "error.h"
#include "instances.h"
#include "common.h"

typedef struct scene
{
	mesh* meshes;
	size_t meshes_count;
	instances instances;

	camera camera;

	light* d_lights;
	size_t d_lights_count;
	texture* d_textures;
	material* d_materials;
	ch_infos ch_infos;

	OptixTraversableHandle ias_hnd;
	CHIZEN_RESULT result;
} scene;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
	scene scene_create_from_gltf(cgltf_data* gltf_data, const OptixProgramGroup ch_od_pg, const OptixProgramGroup ch_ld_pg, const OptixDeviceContext ctx, const cudaStream_t stream);
	CHIZEN_RESULT scene_destroy(scene s);
#ifdef __cplusplus
}
#endif // __cplusplus

