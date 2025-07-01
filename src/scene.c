#include "scene.h"
#include <string.h>
#include <cgltf/cgltf.h>

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

        if (curr_node->mesh != NULL) {
            if (s.meshes == NULL)
            {
                s.meshes = malloc(sizeof(mesh));
            }
            else {
                s.meshes = realloc(s.meshes, (s.meshes_count + 1) * sizeof(mesh));
            }

            cgltf_mesh* curr_mesh = curr_node->mesh;

            s.meshes[s.meshes_count].name = malloc(strlen(curr_mesh->name) + 1);
            strcpy(s.meshes[s.meshes_count].name, curr_mesh->name);

            s.meshes[s.meshes_count].prims_count = curr_mesh->primitives_count;
            s.meshes[s.meshes_count].prims = malloc(sizeof(primitive) * curr_mesh->primitives_count);
            memset(s.meshes[s.meshes_count].prims, 0, sizeof(primitive) * curr_mesh->primitives_count);

            for (size_t p = 0; p < curr_mesh->primitives_count; ++p)
            {
                cgltf_primitive* curr_prim = curr_mesh->primitives + p;

                for (size_t a = 0; a < curr_prim->attributes_count; ++a)
                {
                    cgltf_attribute* curr_attr = curr_prim->attributes + a;

                    if (strcmp(curr_attr->name, "POSITION") == 0)
                    {
                        s.meshes[s.meshes_count].prims[p].positions_count = curr_attr->data->count;
                        s.meshes[s.meshes_count].prims[p].positions = malloc(curr_attr->data->count * curr_attr->data->stride);
                        memcpy(s.meshes[s.meshes_count].prims[p].positions, (void*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->count * curr_attr->data->stride);
                    }
                    else if (strcmp(curr_attr->name, "NORMAL") == 0)
                    {
                        s.meshes[s.meshes_count].prims[p].normals_count = curr_attr->data->count;
                        s.meshes[s.meshes_count].prims[p].normals = malloc(curr_attr->data->count * curr_attr->data->stride);
                        memcpy(s.meshes[s.meshes_count].prims[p].normals, (void*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->count * curr_attr->data->stride);
                    }
                    else if (strcmp(curr_attr->name, "TEXCOORD_0") == 0)
                    {
                        s.meshes[s.meshes_count].prims[p].uvs_count = curr_attr->data->count;
                        s.meshes[s.meshes_count].prims[p].uvs = malloc(curr_attr->data->count * curr_attr->data->stride);
                        memcpy(s.meshes[s.meshes_count].prims[p].uvs, (void*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->count * curr_attr->data->stride);
                    }
                }
            }

            ++s.meshes_count;
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

                s.camera = camera_create(pos, rot, fov, znear, zfar, aspect_ratio, curr_node->camera->name);
            }
            else {
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
            if (s.meshes[m].name != NULL)
            {
                free(s.meshes[m].name);
                s.meshes[m].name = NULL;
            }

            for (size_t p = 0; p < s.meshes[m].prims_count; ++p)
            {
                if (s.meshes[m].prims[p].positions != NULL)
                {
                    free(s.meshes[m].prims[p].positions);
                    s.meshes[m].prims[p].positions = NULL;
                    s.meshes[m].prims[p].positions_count = 0;
                }

                if (s.meshes[m].prims[p].normals != NULL)
                {
                    free(s.meshes[m].prims[p].normals);
                    s.meshes[m].prims[p].normals = NULL;
                    s.meshes[m].prims[p].normals_count = 0;
                }

                if (s.meshes[m].prims[p].uvs != NULL)
                {
                    free(s.meshes[m].prims[p].uvs);
                    s.meshes[m].prims[p].uvs = NULL;
                    s.meshes[m].prims[p].uvs_count = 0;
                }
            }
        }

        free(s.meshes);
        s.meshes = NULL;
    }

    s.meshes_count = 0;

    camera_destroy(s.camera);
}
