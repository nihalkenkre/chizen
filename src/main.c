#include <stdio.h>
#include <Shlwapi.h>

#include "common/renderer.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#define CGLM_IMPLEMENTATION
#include <cglm/include/cglm/cglm.h>

#define STB_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

const float RENDER_HEIGHT = 720.f;
const float ASPECT_RATIO = 16.f / 9.f;
const uint8_t NUM_SAMPLES = 32;

typedef enum render_mode {
    CPU,
    OPTIX
} render_mode;

int main(int argc, char** argv)
{
    uint8_t* pixels = NULL;
    render_mode rm = CPU;

    printf("Hello World\n");

    char* gltf_path = NULL;
    if (argc == 2)
    {
        gltf_path = argv[1];
    }
    else if (argc == 3)
    {
        gltf_path = argv[2];

        if (strcmp(argv[1], "--optix") == 0)
        {
            rm = OPTIX;
        }
    }
    else
    {
        printf("Usage: chizen.exe <//--optix> <gltf_path>\n");
        goto shutdown;
    }

    char* ext = PathFindExtensionA(gltf_path);
    if (strcmp(ext, ".glb") != 0 && strcmp(ext, ".gltf") != 0)
    {
        printf("Only GLTF files supported... Exiting...");
        goto shutdown;
    }

    const float RENDER_WIDTH = RENDER_HEIGHT * ASPECT_RATIO;
    pixels = malloc((size_t)(RENDER_WIDTH * RENDER_HEIGHT) * 4);
    if (pixels == NULL)
        goto shutdown;

    memset(pixels, 0, (size_t)(RENDER_WIDTH * RENDER_HEIGHT) * 4);

    if (rm == CPU)
    {
        renderer_render_cpu((size_t)RENDER_WIDTH, (size_t)RENDER_HEIGHT, NUM_SAMPLES, gltf_path, pixels);
    }
    else if (rm == OPTIX)
    {
#define OPTIX
        renderer_render_optix((size_t)RENDER_WIDTH, (size_t)RENDER_HEIGHT, NUM_SAMPLES, gltf_path, pixels);
    }

    char img_path[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA(NULL), img_path, MAX_PATH);
    PathRemoveFileSpecA(img_path);
    strcat(img_path, "\\test.png");

    stbi_write_png(img_path, (int)RENDER_WIDTH, (int)RENDER_HEIGHT, 4, pixels, 0);

shutdown:
    if (pixels != NULL)
        free(pixels);

    printf("Bye World\n");
    return 0;
}
