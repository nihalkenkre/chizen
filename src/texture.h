#pragma once

#include "image.h"
#include <stdbool.h>
#include <cgltf/cgltf.h>
#include <optix.h>
#include <cuda_runtime.h>
#include "error.h"

typedef struct texture
{
	cudaTextureObject_t d_obj;
	CHIZEN_RESULT result;
} texture;

texture texture_create(const cgltf_data* gltf_data, const cgltf_texture* curr_tex, const image* images);
void texture_destroy(texture t);
