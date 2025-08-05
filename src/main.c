#include <stdio.h>
#include <Shlwapi.h>

#include "renderer.h"
#include "exr.h"
#include "utils.h"

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

	char* file_path = NULL;
	if (argc == 2)
	{
		file_path = argv[1];
	}
	else
	{
		printf("Usage: chizen.exe <gltf_path>\n");
		goto shutdown;
	}

	char* ext = PathFindExtensionA(file_path);
	if (strcmp(ext, ".glb") != 0 && strcmp(ext, ".gltf") != 0)
	{
		printf("Only GLTF files supported... Exiting...");
		goto shutdown;
	}

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

	renderer_render_gltf((size_t)RENDER_WIDTH, (size_t)RENDER_HEIGHT, NUM_SAMPLES, file_path, passes, _countof(passes));

	char img_path[MAX_PATH];
	GetModuleFileNameA(GetModuleHandleA(NULL), img_path, MAX_PATH);
	PathRemoveFileSpecA(img_path);
	strcat(img_path, "\\test.exr");

	write_exr(img_path, (size_t)RENDER_WIDTH, (size_t)RENDER_HEIGHT, passes, _countof(passes));

	for (size_t p = 0; p < _countof(passes); ++p)
	{
		free(passes[p].pixels);
	}

shutdown:

	printf("Bye World\n");
	return 0;
}
