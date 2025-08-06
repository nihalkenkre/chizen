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
const uint8_t NUM_SAMPLES = 32;

int main(int argc, char** argv)
{
	printf("Hello World\n");
	CHIZEN_RESULT result = CHIZEN_RESULT_SUCCESS;
	const float RENDER_WIDTH = RENDER_HEIGHT * ASPECT_RATIO;

	exr_pass passes[] =
	{
		{
			.layer = {
				.type = EXR_LAYER_TYPE_BASECOLOR,
				.name = "BaseColor",
			},
			.pixels = malloc((size_t)(RENDER_WIDTH * RENDER_HEIGHT * 4 * sizeof(float))),
		},
		{
			.layer = {
				.type = EXR_LAYER_TYPE_NORMAL,
				.name = "Normal",
			},
			.pixels = malloc((size_t)(RENDER_WIDTH * RENDER_HEIGHT * 4 * sizeof(float))),
		},
		{
			.layer = {
				.type = EXR_LAYER_TYPE_UV,
				.name = "UV",
			},
			.pixels = malloc((size_t)(RENDER_WIDTH * RENDER_HEIGHT * 4 * sizeof(float))),
		},
		{
			.layer = {
				.type = EXR_LAYER_TYPE_METALNESS,
				.name = "Metalness",
			},
			.pixels = malloc((size_t)(RENDER_WIDTH * RENDER_HEIGHT * 4 * sizeof(float))),
		},
		{
			.layer = {
				.type = EXR_LAYER_TYPE_ROUGHNESS,
				.name = "Roughness",
			},
			.pixels = malloc((size_t)(RENDER_WIDTH * RENDER_HEIGHT * 4 * sizeof(float))),
		},
	};

	char* file_path = NULL;
	if (argc == 2)
	{
		file_path = argv[1];
	}
	else
	{
		printf("Usage: chizen.exe <gltf_path>\n");
		result = CHIZEN_RESULT_USAGE_ERROR;
		goto shutdown;
	}

	char* ext = PathFindExtensionA(file_path);
	if (strcmp(ext, ".glb") != 0 && strcmp(ext, ".gltf") != 0)
	{
		printf("Only GLTF files supported... Exiting...");
		result = CHIZEN_RESULT_FILE_TYPE_ERROR;
		goto shutdown;
	}


	CHIZEN_RESULT_CHECK(renderer_render_gltf((size_t)RENDER_WIDTH, (size_t)RENDER_HEIGHT, NUM_SAMPLES, file_path, passes, _countof(passes)), result);

	char img_path[MAX_PATH];
	GetModuleFileNameA(GetModuleHandleA(NULL), img_path, MAX_PATH);
	PathRemoveFileSpecA(img_path);
	strcat(img_path, "\\test.exr");

	CHIZEN_RESULT_CHECK(write_exr(img_path, (size_t)RENDER_WIDTH, (size_t)RENDER_HEIGHT, passes, _countof(passes)), result);

shutdown:

	for (size_t p = 0; p < _countof(passes); ++p)
	{
		free(passes[p].pixels);
	}

	printf("Bye World\n");
	return 0;
}
