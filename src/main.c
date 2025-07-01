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

const float RENDER_HEIGHT = 720.f;
const float ASPECT_RATIO = 16.f / 9.f;
const uint8_t NUM_SAMPLES = 4;


int main(int argc, char** argv)
{
    uint8_t* pixels = NULL;
    scene scene = { 0 };

    printf("Hello World\n");
    if (argc != 2)
    {
        printf("USAGE: chizen.exe <gltf_path>\n");
        goto shutdown;
    }

    char* file_path = argv[1];
    char* ext = PathFindExtensionA(file_path);

    if (strcmp(ext, ".glb") == 0 || strcmp(ext, ".gltf") == 0) {
        scene = scene_parse_gltf(file_path);
    }
    else {
        printf("Only GLTF files supported at the moment...\n");
        goto shutdown;
    }

    const float RENDER_WIDTH = RENDER_HEIGHT * ASPECT_RATIO;
    pixels = malloc((size_t)(RENDER_WIDTH * RENDER_HEIGHT) * 4);

    renderer_render(RENDER_WIDTH, RENDER_HEIGHT, NUM_SAMPLES, scene, pixels);

    char img_path[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA(NULL), img_path, MAX_PATH);
    PathRemoveFileSpecA(img_path);
    strcat(img_path, "\\test.png");

    stbi_write_png(img_path, (int)RENDER_WIDTH, (int)RENDER_HEIGHT, 4, pixels, 0);

shutdown:
    if (pixels != NULL)
        free(pixels);

    scene_destroy(scene);

    printf("Bye World\n");
    return 0;
}