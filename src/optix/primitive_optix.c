#include "primitive_optix.h"

#include <stdlib.h>
#include <string.h>

static inline void CU_CHECK(const char* action, const cudaError_t result)
{
    if (result > cudaSuccess)
    {
        printf("CUDA ERR %d: %s\nExiting...\n", result, action);
        exit(result);
    }
}

static inline void OPTIX_CHECK(const char* action, const OptixResult result)
{
    if (result > OPTIX_SUCCESS)
    {
        printf("ERR: %s %s\n", action, optixGetErrorName(result));
        exit(result);
    }
}

primitive_optix primitive_optix_create(cgltf_primitive* curr_prim, mat4 node_xform, const OptixDeviceContext ctx, const cudaStream_t stream)
{
    primitive_optix p = { 0 };

    if (curr_prim->material != NULL)
    {
        p.material = material_create(curr_prim->material);
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

    p.tris_count = indices_count / 3;
    p.tris = malloc(p.tris_count * sizeof(triangle));

    size_t triangle_index = 0;
    for (size_t i = 0; i < indices_count; i += 3)
    {
        p.tris[triangle_index++] = triangle_create(node_xform, positions, normals, uvs, indices, i);
    }

    free(indices);
    indices = NULL;

    CU_CHECK("alloc primitive vertex buffer", cudaMalloc((void**)&p.vertex_buffer, sizeof(triangle) * p.tris_count));

    unsigned int flags = OPTIX_GEOMETRY_FLAG_NONE;

    OptixBuildInput build_input = {
        .type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES,
        .triangleArray = {
            .numVertices = (unsigned int)(p.tris_count * 3),
            .vertexBuffers = &p.vertex_buffer,
            .vertexStrideInBytes = sizeof(triangle),
            .vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3,
            .numSbtRecords = 1,
            .flags = &flags,
        },
    };
    OptixAccelBuildOptions build_options = {
        .operation = OPTIX_BUILD_OPERATION_BUILD,
        .buildFlags = OPTIX_BUILD_FLAG_ALLOW_COMPACTION,
    };

    OptixAccelBufferSizes buffer_sizes = { 0 };
    OPTIX_CHECK("compute primitive accel build sizes", optixAccelComputeMemoryUsage(ctx, &build_options, &build_input, 1, &buffer_sizes));

    //optixAccelBuild(ctx, stream, &build_options, &build_input, 1, 

    return p;
}

void primitive_optix_destroy(primitive_optix p)
{
    if (p.tris != NULL)
    {
        free(p.tris);
        p.tris = NULL;
        p.tris_count = 0;
    }

    material_destroy(p.material);
    CU_CHECK("free primitive vertex buffer", cudaFree((void*)p.vertex_buffer));
}
