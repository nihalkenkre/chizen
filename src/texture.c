#include "texture.h"
#include "utils.h"
#include <stb/stb_image.h>

texture texture_create(const cgltf_data* gltf_data, const cgltf_texture* curr_tex, const image* images)
{
	const struct cudaResourceDesc tex_res_desc = {
		.res = {
			.array = {
				.array = images[cgltf_image_index(gltf_data, curr_tex->image)].d_pixel_array,
			},
		},
		.resType = cudaResourceTypeArray,
	};

	texture t = { 0 };
	enum cudaTextureAddressMode address_mode_s = cudaAddressModeWrap;
	enum cudaTextureAddressMode address_mode_t = cudaAddressModeWrap;

	switch (curr_tex->sampler->wrap_s)
	{
	case cgltf_wrap_mode_repeat:
		address_mode_s = cudaAddressModeWrap;
		break;

	case cgltf_wrap_mode_clamp_to_edge:
		address_mode_s = cudaAddressModeClamp;
		break;

	case cgltf_wrap_mode_mirrored_repeat:
		address_mode_s = cudaAddressModeMirror;
		break;

	default:
		break;
	}

	switch (curr_tex->sampler->wrap_t)
	{
	case cgltf_wrap_mode_repeat:
		address_mode_t = cudaAddressModeWrap;
		break;

	case cgltf_wrap_mode_clamp_to_edge:
		address_mode_t = cudaAddressModeClamp;
		break;

	case cgltf_wrap_mode_mirrored_repeat:
		address_mode_t = cudaAddressModeMirror;
		break;

	default:
		break;
	}

	const struct cudaTextureDesc tex_desc = {
		.addressMode = {address_mode_s, address_mode_t, 0},
		.normalizedCoords = true,
	};

	CU_CHECK("create texture object", cudaCreateTextureObject(&t.d_obj, &tex_res_desc, &tex_desc, NULL));

	return t;
}

void texture_destroy(texture t)
{
	CU_CHECK("destroy texture object", cudaDestroyTextureObject(t.d_obj));
}
