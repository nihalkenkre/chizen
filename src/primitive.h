#pragma once

#include "common.h"
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

	OptixBuildInput build_input;
	CHIZEN_RESULT result;
} primitive;

primitive primitive_create(const cgltf_data* gltf_data, cgltf_primitive* curr_prim, const OptixProgramGroup ch_rg_pg, const OptixProgramGroup ch_sr_pg, const OptixDeviceContext ctx, const cudaStream_t stream, ch_infos* ch_infos);
CHIZEN_RESULT primitive_destroy(primitive p);
