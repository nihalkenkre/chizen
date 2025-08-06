#pragma once

#include "texture.h"
#include <cgltf/cgltf.h>
#include <optix.h>
#include <cuda_runtime.h>
#include "error.h"

typedef struct material
{
	float4 base_color_factor;
	float metalness_factor;
	float roughness_factor;
	int32_t base_tex_idx;
	int32_t mr_tex_idx;
	CHIZEN_RESULT result;
} material;

material material_create(const cgltf_data* gltf_data, cgltf_material* curr_mat, texture* textures);
