#pragma once

#include <cgltf/cgltf.h>
#include <cglm/include/cglm/cglm.h>
#include "material.h"
#include "error.h"

typedef struct custom_gas_data
{
	__align__(OPTIX_ACCEL_BUFFER_BYTE_ALIGNMENT)
		float3* normals;
	float2* uvs;
	void* indices;
	OptixIndicesFormat indices_format;
	int32_t material_index;
} custom_gas_data;

typedef struct primitive
{
	CUdeviceptr d_positions;
	CUdeviceptr d_normals;
	CUdeviceptr d_uvs;
	CUdeviceptr d_indices;
	OptixTraversableHandle gas_hnd;
	CUdeviceptr d_gas_op_buffer;
	CHIZEN_RESULT result;
} primitive;

primitive primitive_create(const cgltf_data* data, cgltf_primitive* curr_prim, const OptixDeviceContext ctx, const cudaStream_t stream);
void primitive_destroy(primitive p);
