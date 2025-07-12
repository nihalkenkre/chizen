#include "primitive.h"
#include <string.h>
#include <stdlib.h>

primitive primitive_create(const cgltf_primitive* curr_prim, mat4 node_xform)
{
    primitive prim = {
        .bbox = bbox_create(),
    };

    if (curr_prim->material != NULL)
    {
        cgltf_material* curr_mat = curr_prim->material;

        prim.material = material_create(curr_mat);
    }

    size_t indices_count = curr_prim->indices->count;
    uint32_t* indices = malloc(curr_prim->indices->count * sizeof(uint32_t));

    if (curr_prim->indices->component_type == cgltf_component_type_r_32u)
    {
        memcpy(indices, (void*)((size_t)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->buffer_view->offset + curr_prim->indices->offset), indices_count * sizeof(uint32_t));
    }
    else if (curr_prim->indices->component_type == cgltf_component_type_r_16u)
    {
        uint16_t* idxs = (uint16_t*)((size_t)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->buffer_view->offset + curr_prim->indices->offset);

        for (size_t i = 0; i < indices_count; ++i)
        {
            indices[i] = idxs[i];
        }
    }

    vec3* positions = NULL;
    vec3* normals = NULL;
    vec2* uvs = NULL;

    for (size_t a = 0; a < curr_prim->attributes_count; ++a)
    {
        cgltf_attribute* curr_attr = curr_prim->attributes + a;

        if (strcmp(curr_attr->name, "POSITION") == 0)
        {
            positions = (vec3*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset);
        }
        else if (strcmp(curr_attr->name, "NORMAL") == 0)
        {
            normals = (vec3*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset);
        }
        else if (strcmp(curr_attr->name, "TEXCOORD_0") == 0)
        {
            uvs = (vec2*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset);
        }
    }

    prim.tris_count = indices_count / 3;
    prim.tris = malloc(prim.tris_count * sizeof(triangle));

    size_t triangle_index = 0;
    for (size_t i = 0; i < indices_count; i += 3)
    {
        prim.tris[triangle_index++] = triangle_create(node_xform, positions, normals, uvs, indices, i);
        bbox_expand_to_tri(&prim.bbox, prim.tris[triangle_index - 1]);
    }

    free(indices);
    indices = NULL;

    return prim;
}

void primitive_destroy(primitive prim)
{
    if (prim.tris != NULL)
    {
        free(prim.tris);
        prim.tris = NULL;
        prim.tris_count = 0;
    }

    material_destroy(prim.material);
}
