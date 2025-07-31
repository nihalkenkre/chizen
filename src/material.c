#include "material.h"
#include <string.h>
#include <stdio.h>

material material_create(const cgltf_data* gltf_data, cgltf_material* curr_mat, texture* textures)
{
	material m = {
		.base_texture_index = -1,
	};

	if (curr_mat->has_pbr_metallic_roughness)
	{
		cgltf_pbr_metallic_roughness pbr_mr = curr_mat->pbr_metallic_roughness;
		memcpy(&m.base_color, pbr_mr.base_color_factor, sizeof(m.base_color));
		if (pbr_mr.base_color_texture.texture != NULL)
		{
			m.base_texture_index = (int32_t)cgltf_texture_index(gltf_data, pbr_mr.base_color_texture.texture);
		}
	}

	return m;
}
