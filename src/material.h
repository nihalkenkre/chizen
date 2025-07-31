#pragma once

#include "texture.h"
#include <cgltf/cgltf.h>
#include <optix.h>
#include <cuda_runtime.h>

typedef struct material
{
	int32_t base_texture_index;
	float4 base_color;
} material;

material material_create(const cgltf_data* gltf_data, cgltf_material* curr_mat, texture* textures);
