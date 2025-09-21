#include "material.h"
#include <string.h>
#include <stdio.h>

material material_create(const cgltf_data* gltf_data, cgltf_material* curr_mat, texture* textures)
{
	material m = {
		.base_tex_idx = -1,
		.mr_tex_idx = -1,
		.emissive_tex_idx = -1,
	};

	if (curr_mat->has_pbr_metallic_roughness)
	{
		cgltf_pbr_metallic_roughness pbr_mr = curr_mat->pbr_metallic_roughness;
		memcpy(&m.base_color_factor, pbr_mr.base_color_factor, sizeof(m.base_color_factor));
		if (pbr_mr.base_color_texture.texture != NULL)
		{
			m.base_tex_idx = (int32_t)cgltf_texture_index(gltf_data, pbr_mr.base_color_texture.texture);
		}

		m.metalness_factor = pbr_mr.metallic_factor;
		if (pbr_mr.metallic_roughness_texture.texture != NULL)
		{
			m.mr_tex_idx = (int32_t)cgltf_texture_index(gltf_data, pbr_mr.metallic_roughness_texture.texture);
		}
		m.roughness_factor = pbr_mr.roughness_factor;
	}

	if (curr_mat->emissive_texture.texture != NULL)
	{
		m.emissive_tex_idx = (int32_t)cgltf_texture_index(gltf_data, curr_mat->emissive_texture.texture);
	}
	memcpy(&m.emissive_factor, curr_mat->emissive_factor, sizeof(m.emissive_factor));

	if (curr_mat->has_emissive_strength)
	{
		m.emissive_factor.x += (curr_mat->emissive_strength.emissive_strength - m.emissive_factor.x);
		m.emissive_factor.y += (curr_mat->emissive_strength.emissive_strength - m.emissive_factor.y);
		m.emissive_factor.z += (curr_mat->emissive_strength.emissive_strength - m.emissive_factor.z);
	}

	return m;
}
