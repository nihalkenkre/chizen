#include "primitive.h"
#include <string.h>
#include <stdlib.h>

primitive primitive_create(const cgltf_primitive* curr_prim, mat4 node_xform)
{
    primitive prim = {
        .bbox = bbox_create(),
        .indices_count = curr_prim->indices->count,
        .indices = malloc(curr_prim->indices->count * sizeof(uint32_t)),
    };

    if (curr_prim->material != NULL)
    {
        cgltf_material* curr_mat = curr_prim->material;

        prim.material = material_create(curr_mat);
    }

    if (curr_prim->indices->component_type == cgltf_component_type_r_32u)
    {
        memcpy(prim.indices, (void*)((size_t)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->buffer_view->offset + curr_prim->indices->offset), prim.indices_count * sizeof(uint32_t));
    }
    else if (curr_prim->indices->component_type == cgltf_component_type_r_16u)
    {
        uint16_t* idxs = (uint16_t*)((size_t)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->buffer_view->offset + curr_prim->indices->offset);

        for (size_t i = 0; i < prim.indices_count; ++i)
        {
            prim.indices[i] = idxs[i];
        }
    }

    for (size_t a = 0; a < curr_prim->attributes_count; ++a)
    {
        cgltf_attribute* curr_attr = curr_prim->attributes + a;

        if (strcmp(curr_attr->name, "POSITION") == 0)
        {
            prim.positions_count = curr_attr->data->count;
            prim.positions = malloc(curr_attr->data->count * curr_attr->data->stride);
            if (prim.positions != NULL)
                memcpy(prim.positions, (void*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->count * curr_attr->data->stride);
            else {
                printf("ERR: Could not allocate %lld\nExiting...\n", curr_attr->data->count * curr_attr->data->stride);
                exit(0);
            }

            for (size_t p = 0; p < prim.positions_count; ++p)
            {
                glm_mat4_mulv3(node_xform, prim.positions[p], 1.f, prim.positions[p]);
                bbox_expand_to(&prim.bbox, prim.positions[p]);
            }
        }
        else if (strcmp(curr_attr->name, "NORMAL") == 0)
        {
            prim.normals_count = curr_attr->data->count;
            prim.normals = malloc(curr_attr->data->count * curr_attr->data->stride);
            if (prim.normals != NULL)
                memcpy(prim.normals, (void*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->count * curr_attr->data->stride);
            else {
                printf("ERR: Could not allocate %lld\nExiting...\n", curr_attr->data->count * curr_attr->data->stride);
                exit(0);
            }
        }
        else if (strcmp(curr_attr->name, "TEXCOORD_0") == 0)
        {
            prim.uvs_count = curr_attr->data->count;
            prim.uvs = malloc(curr_attr->data->count * curr_attr->data->stride);
            if (prim.uvs != NULL)
                memcpy(prim.uvs, (void*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->count * curr_attr->data->stride);
            else {
                printf("ERR: Could not allocate %lld\nExiting...\n", curr_attr->data->count * curr_attr->data->stride);
                exit(0);
            }
        }
    }

    prim.triangles_count = prim.indices_count / 3;
    prim.triangles = malloc(prim.triangles_count * sizeof(triangle));

    size_t triangle_index = 0;
    for (size_t i = 0; i < prim.indices_count; i += 3)
    {
        prim.triangles[triangle_index++] = triangle_create(prim.positions, prim.normals, prim.uvs, prim.indices, i);
    }

    return prim;
}

void primitive_destroy(primitive prim)
{
    if (prim.positions != NULL) {
        free(prim.positions);
        prim.positions = NULL;
        prim.positions_count = 0;
    }

    if (prim.normals != NULL)
    {
        free(prim.normals);
        prim.normals = NULL;
        prim.normals_count = 0;
    }

    if (prim.uvs != NULL)
    {
        free(prim.uvs);
        prim.uvs = NULL;
        prim.uvs_count = 0;
    }

    if (prim.indices != NULL)
    {
        free(prim.indices);
        prim.indices = NULL;
        prim.indices_count = 0;
    }

    if (prim.triangles != NULL)
    {
        free(prim.triangles);
        prim.triangles = NULL;
        prim.triangles_count = 0;
    }

    material_destroy(prim.material);
}
