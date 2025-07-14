#pragma once

#include <optix.h>
#include <cuda_runtime.h>
#include <cgltf/cgltf.h>
#include <cglm/include/cglm/cglm.h>

typedef struct primitive_optix
{
	vec3* positions;
	size_t positions_count;

	vec3* normals;
	size_t normals_count;

	vec2* uvs;
	size_t uvs_count;

	uint32_t* indices;
	size_t indices_count;
} primitive_optix;

primitive_optix primitive_optix_create(cgltf_primitive* curr_prim, mat4 node_xform, const OptixDeviceContext ctx, const cudaStream_t stream);
void primitive_optix_destroy(primitive_optix p);
