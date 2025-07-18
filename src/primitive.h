#pragma once

#include <optix.h>
#include <cuda_runtime.h>
#include <cgltf/cgltf.h>
#include <cglm/include/cglm/cglm.h>

typedef struct primitive
{
	vec3* positions;
	size_t positions_count;

	vec3* normals;
	size_t normals_count;

	vec2* uvs;
	size_t uvs_count;

	uint32_t* indices;
	size_t indices_count;

	CUdeviceptr d_positions;
	CUdeviceptr d_indices;
	OptixTraversableHandle gas_hnd;
	CUdeviceptr d_gas_op_buffer;
} primitive;

primitive primitive_create(cgltf_primitive* curr_prim, const OptixDeviceContext ctx, const cudaStream_t stream);
void primitive_destroy(primitive p);
