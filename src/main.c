#include <stdio.h>
#include <Shlwapi.h>

#include "renderer.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#define CGLM_IMPLEMENTATION
#include <cglm/include/cglm/cglm.h>

#define STB_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include "misc.hpp"

const float RENDER_HEIGHT = 720.f;
const float ASPECT_RATIO = 16.f / 9.f;
const uint8_t NUM_SAMPLES = 32;

int main(int argc, char** argv)
{
	printf("Hello World\n");

	char* gltf_path = NULL;
	if (argc == 2)
	{
		gltf_path = argv[1];
	}
	else
	{
		printf("Usage: chizen.exe <gltf_path>\n");
		goto shutdown;
	}

	char* ext = PathFindExtensionA(gltf_path);
	if (strcmp(ext, ".glb") != 0 && strcmp(ext, ".gltf") != 0)
	{
		printf("Only GLTF files supported... Exiting...");
		goto shutdown;
	}

	const float RENDER_WIDTH = RENDER_HEIGHT * ASPECT_RATIO;

	const exr_pass passes[] = {
		{
			.layer = EXR_LAYER_NORMAL,
			.pixels = malloc((size_t)(RENDER_WIDTH * RENDER_HEIGHT * 4 * sizeof(float))),
		},
		{
			.layer = EXR_LAYER_UV,
			.pixels = malloc((size_t)(RENDER_WIDTH * RENDER_HEIGHT * 4 * sizeof(float))),
		},
	};

	float** passes_pixels = malloc(sizeof(float*) * _countof(passes));

	for (size_t pp = 0; pp < _countof(passes); ++pp)
	{
		passes_pixels[pp] = passes[pp].pixels;
	}

	renderer_render((size_t)RENDER_WIDTH, (size_t)RENDER_HEIGHT, NUM_SAMPLES, gltf_path, passes_pixels, _countof(passes));

	char img_path[MAX_PATH];
	GetModuleFileNameA(GetModuleHandleA(NULL), img_path, MAX_PATH);
	PathRemoveFileSpecA(img_path);
	strcat(img_path, "\\test.exr");

	write_exr(img_path, (size_t)RENDER_WIDTH, (size_t)RENDER_HEIGHT, passes, _countof(passes));

	for (size_t p = 0; p < _countof(passes); ++p)
	{
		free(passes[p].pixels);
	}

	free(passes_pixels);

shutdown:

	printf("Bye World\n");
	return 0;
}
