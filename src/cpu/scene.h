#pragma once

#include "../common/camera.h"
#include "primitive.h"
#include "accel.h"

typedef struct scene
{
	//    mesh* meshes;
	//    size_t meshes_count;

	primitive* prims;
	size_t prims_count;

	accel accel;

	camera camera;
} scene;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	scene scene_create(const char* gltf_path);
	void scene_destroy(scene s);

#ifdef __cplusplus
}
#endif // __cplusplus

