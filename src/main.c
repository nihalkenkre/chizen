#include <stdio.h>
#include <Shlwapi.h>

#include "renderer.h"
#include "exr.h"
#include "error.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#define CGLM_IMPLEMENTATION
#include <cglm/include/cglm/cglm.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

const float RENDER_HEIGHT = 720.f;
const float ASPECT_RATIO = 16.f / 9.f;

int main(int argc, char** argv)
{
	printf("Hello World\n");
	CHIZEN_RESULT chi_result = CHIZEN_RESULT_SUCCESS;
	const float RENDER_WIDTH = RENDER_HEIGHT * ASPECT_RATIO;

	exr_pass passes[] =
	{
		{
			.layer = {
				.type = EXR_LAYER_TYPE_DIFFUSE,
				.name = "Diffuse",
				.num_channels = 4,
			},
		},
		{
			.layer = {
				.type = EXR_LAYER_TYPE_SPECULAR,
				.name = "Specular",
				.num_channels = 4,
			},
		},
		{
			.layer = {
				.type = EXR_LAYER_TYPE_BASECOLOR,
				.name = "BaseColor",
				.num_channels = 4,
			},
		},
		{
			.layer = {
				.type = EXR_LAYER_TYPE_NORMAL,
				.name = "Normal",
				.num_channels = 3,
			},
		},
		{
			.layer = {
				.type = EXR_LAYER_TYPE_UV,
				.name = "UV",
				.num_channels = 2,
			},
		},
		// {
		// 	.layer = {
		// 		.type = EXR_LAYER_TYPE_METALNESS,
		// 		.name = "Metalness",
		// 		.num_channels = 1,
		// 	},
		// },
		// {
		// 	.layer = {
		// 		.type = EXR_LAYER_TYPE_ROUGHNESS,
		// 		.name = "Roughness",
		// 		.num_channels = 1,
		// 	},
		// },
		// {
		// 	.layer = {
		// 		.type = EXR_LAYER_TYPE_ZDEPTH,
		// 		.name = "ZDepth",
		// 		.num_channels = 1,
		// 	},
		// },
		// {
		// 	.layer = {
		// 		.type = EXR_LAYER_TYPE_IRRADIANCE,
		// 		.name = "Irradiance",
		// 		.num_channels = 1,
		// 	},
		// },
	};

	for (size_t p = 0; p < _countof(passes); ++p)
	{
		passes[p].pixels = calloc(1, (size_t)(RENDER_WIDTH * RENDER_HEIGHT * passes[p].layer.num_channels * sizeof(float)));
		if (passes[p].pixels == NULL)
		{
			printf("calloc failed for passes[%lld].pixels\n", p);
			chi_result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
			goto cpu_error;
		}
	}

	char* file_path = NULL;
	if (argc == 2)
	{
		file_path = argv[1];
	}
	else
	{
		printf("Usage: chizen.exe <gltf_path>\n");
		chi_result = CHIZEN_RESULT_USAGE_ERROR;
		goto cpu_error;
	}

	char* ext = PathFindExtensionA(file_path);
	if (strcmp(ext, ".glb") != 0 && strcmp(ext, ".gltf") != 0)
	{
		printf("Only GLTF files supported... Exiting...");
		chi_result = CHIZEN_RESULT_FILE_ERROR;
		goto cpu_error;
	}

	CHIZEN_RESULT_CHECK("renderer render gltf", renderer_render_gltf((size_t)RENDER_WIDTH, (size_t)RENDER_HEIGHT, file_path, passes, _countof(passes)), chi_result);

	char img_path[MAX_PATH];
	GetModuleFileNameA(GetModuleHandleA(NULL), img_path, MAX_PATH);
	PathRemoveFileSpecA(img_path);
	strcat(img_path, "\\test.exr");

	CHIZEN_RESULT_CHECK("write exr", write_exr(img_path, (size_t)RENDER_WIDTH, (size_t)RENDER_HEIGHT, passes, _countof(passes)), chi_result);

cpu_error:
gpu_error:
	for (size_t p = 0; p < _countof(passes); ++p)
	{
		free(passes[p].pixels);
	}

	printf("Bye World\n");
	return 0;
}
