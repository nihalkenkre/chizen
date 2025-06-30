#include "scene.h"

#include <cgltf/cgltf.h>

scene scene_parse_gltf(const char* gltf_path)
{
    scene scene = { 0 };
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

        if (curr_node->mesh != NULL) {
            ++scene.meshes_count;

            if (scene.meshes == NULL)
            {
                scene.meshes = malloc(scene.meshes_count * sizeof(mesh));
            }
            else {
                scene.meshes = realloc(scene.meshes, scene.meshes_count * sizeof(mesh));
            }

            cgltf_mesh* curr_mesh = curr_node->mesh;
        }
        else if (curr_node->camera != NULL)
        {
            if (curr_node->camera->type == cgltf_camera_type_perspective)
            {
                float aspect_ratio = curr_node->camera->data.perspective.has_aspect_ratio ? curr_node->camera->data.perspective.aspect_ratio : 16.f / 9.f;
                float fov = curr_node->camera->data.perspective.yfov;
                float znear = curr_node->camera->data.perspective.znear;
                float zfar = curr_node->camera->data.perspective.has_zfar ? curr_node->camera->data.perspective.zfar : 1000.f;

                vec3 pos = { 0 }; vec4 rot = { 0 };
                if (curr_node->has_matrix) {
                    mat4 xform_matrix;
                    glm_mat4_make(curr_node->matrix, xform_matrix);

                    vec3 t; mat4 r; vec3 s;
                    glm_decompose(xform_matrix, t, r, s);
                    glm_vec3_copy(t, pos);
                    glm_mat4_quat(r, rot);
                }
                else {
                    if (curr_node->has_translation)
                    {
                        glm_vec3_copy(curr_node->translation, pos);
                    }
                    if (curr_node->has_rotation)
                    {
                        glm_vec4_copy(curr_node->rotation, rot);
                    }
                }

                scene.camera = camera_create(pos, rot, fov, znear, zfar, aspect_ratio, curr_node->camera->name);
            }
            else {
                printf("Only perspective cameras supported at the moment...\n");
                goto shutdown;
            }
        }
    }

shutdown:
    cgltf_free(gltf_data);

    return scene;
}

void scene_destroy(scene s)
{
    if (s.meshes != NULL)
        free(s.meshes);

    s.meshes_count = 0;

    camera_destroy(s.camera);
}
