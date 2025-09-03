#pragma once

#include "texture.h"
#include <cgltf/cgltf.h>

typedef struct material
{
	float4 base_color_factor;
	float3 emissive_factor;
	float metalness_factor;
	float roughness_factor;
	float emissive_strength;
	int16_t emissive_tex_idx;
	int16_t base_tex_idx;
	int16_t mr_tex_idx;
	CHIZEN_RESULT result;
} material;

material material_create(const cgltf_data* gltf_data, cgltf_material* curr_mat, texture* textures);
