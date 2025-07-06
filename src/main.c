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

typedef enum render_mode {
    CPU,
    CUDA,
    OPTIX
} render_mode;

int main(int argc, char** argv)
{
    uint8_t* pixels = NULL;
    scene scene = { 0 };
    render_mode rm = CPU;

    printf("Hello World\n");
    for (int a = 1; a < argc; ++a)
    {
        if (strcmp(argv[a], "--cuda") == 0)
        {
            rm = CUDA;
        }

        char* ext = PathFindExtensionA(argv[a]);
        if (strcmp(ext, ".glb") == 0 || strcmp(ext, ".gltf") == 0)
        {
            scene = scene_create(argv[a]);
        }
    }

    const float RENDER_WIDTH = RENDER_HEIGHT * ASPECT_RATIO;
    pixels = malloc((size_t)(RENDER_WIDTH * RENDER_HEIGHT) * 4);

    if (rm == CPU)
    {
        renderer_render_cpu(RENDER_WIDTH, RENDER_HEIGHT, NUM_SAMPLES, scene, pixels);

    }
    else if (rm == CUDA)
    {
        renderer_render_cuda((size_t)RENDER_WIDTH, (size_t)RENDER_HEIGHT, NUM_SAMPLES, scene, pixels);
    }

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
