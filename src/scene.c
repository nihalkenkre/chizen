#include "scene.h"
#include "utils.h"
#include <string.h>

scene scene_parse_gltf(const char* gltf_path)
{
    scene s = { 0 };
    cgltf_options gltf_options = { 0 };
    cgltf_data* gltf_data = NULL;

    if (cgltf_parse_file(&gltf_options, gltf_path, &gltf_data) != cgltf_result_success ||
        cgltf_validate(gltf_data) != cgltf_result_success ||
        cgltf_load_buffers(&gltf_options, gltf_data, gltf_path) != cgltf_result_success)
    {
        printf("Error parsing %s\n", gltf_path);
        goto shutdown;
    }

    for (size_t n = 0; n < gltf_data->nodes_count; ++n)
    {
        cgltf_node* curr_node = gltf_data->nodes + n;

        if (curr_node->mesh != NULL)
        {
            if (s.meshes_count == 0)
            {
                s.meshes = malloc(sizeof(mesh));
            }
            else {
                s.meshes = realloc(s.meshes, sizeof(mesh) * (s.meshes_count + 1));
            }

            s.meshes[s.meshes_count++] = mesh_create(curr_node);
        }
        else if (curr_node->camera != NULL)
        {
            if (curr_node->camera->type == cgltf_camera_type_perspective)
            {
                s.camera = camera_create(curr_node);
            }
            else
            {
                printf("Only perspective cameras supported at the moment...\n");
                goto shutdown;
            }
        }
    }

shutdown:
    cgltf_free(gltf_data);

    return s;
}

void scene_destroy(scene s)
{
    if (s.meshes != NULL)
    {
        for (size_t m = 0; m < s.meshes_count; ++m)
        {
            mesh_destroy(s.meshes[m]);
        }

        free(s.meshes);
        s.meshes = NULL;
        s.meshes_count = 0;
    }

    camera_destroy(s.camera);
}
