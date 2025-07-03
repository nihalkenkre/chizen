#include "primitive.h"
#include <string.h>
#include <stdlib.h>

primitive primitive_create(const cgltf_primitive* curr_prim, mat4 node_xform)
{
    primitive prim = { 0 };
    prim.bbox = bbox_create();

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

    return prim;
}

void primitive_destroy(primitive p)
{
    if (p.positions != NULL) {
        free(p.positions);
        p.positions = NULL;
        p.positions_count = 0;
    }

    if (p.normals != NULL)
    {
        free(p.normals);
        p.normals = NULL;
        p.normals_count = 0;
    }

    if (p.uvs != NULL)
    {
        free(p.uvs);
        p.uvs = NULL;
        p.uvs_count = 0;
    }
}
