#include "scene.h"
#include "../common/utils.h"
#include <string.h>

scene scene_create(const char* gltf_path)
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
            cgltf_mesh* curr_mesh = curr_node->mesh;
            
            if (s.prims_count == 0)
            {
                s.prims = malloc(sizeof(primitive) * curr_mesh->primitives_count);
            }
            else 
            {
                s.prims = realloc(s.prims, sizeof(primitive) * (s.prims_count + curr_mesh->primitives_count));
            }

            mat4 node_xform = { 0 };
            utils_get_xform_matrix_for_node(curr_node, node_xform);

            for (size_t p = 0; p < curr_mesh->primitives_count; ++p)
            {
                s.prims[s.prims_count++] = primitive_create(curr_mesh->primitives + p, node_xform);
            }
        }
        else if (curr_node->camera != NULL)
        {
            if (curr_node->camera->type == cgltf_camera_type_perspective)
            {
                s.camera = camera_create(curr_node);
            }
        }
    }

    s.accel = accel_create(s.prims, s.prims_count);

shutdown:
    cgltf_free(gltf_data);

    return s;
}

void scene_destroy(scene s)
{
    if (s.prims != NULL)
    {
        for (size_t m = 0; m < s.prims_count; ++m)
        {
           primitive_destroy(s.prims[m]);
        }

        free(s.prims);
        s.prims = NULL;
        s.prims_count = 0;
    }

    camera_destroy(s.camera);
    accel_destroy(s.accel);
}
