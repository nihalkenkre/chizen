#include "scene_optix.h"

scene_optix scene_optix_create(const char* gltf_path, const OptixDeviceContext ctx, const cudaStream_t stream)
{
    scene_optix s = { 0 };
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
            cgltf_mesh* curr_mesh = curr_node->mesh;

            if (s.meshes_count == 0)
            {
                s.meshes = malloc(sizeof(mesh_optix));
            }
            else
            {
                s.meshes = realloc(s.meshes, sizeof(mesh_optix) * (s.meshes_count + 1));
            }

            s.meshes[s.meshes_count++] = mesh_optix_create(curr_node, ctx, stream);
        }
        else if (curr_node->camera != NULL)
        {
            if (curr_node->camera->type == cgltf_camera_type_perspective)
            {
                s.camera = camera_create(curr_node);
            }
        }
    }

shutdown:
    cgltf_free(gltf_data);

    return s;
}

void scene_optix_destroy(scene_optix s)
{
    if (s.meshes != NULL)
    {
        for (size_t m = 0; m < s.meshes_count; ++m)
        {
            mesh_optix_destroy(s.meshes[m]);
        }

        free(s.meshes);
        s.meshes_count = 0;
    }
}
