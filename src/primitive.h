#pragma once

#include <optix.h>
#include <cuda_runtime.h>
#include <cgltf/cgltf.h>
#include <cglm/include/cglm/cglm.h>

typedef struct primitive
{
	CUdeviceptr d_positions;
	CUdeviceptr d_normals;
	CUdeviceptr d_uvs;
	CUdeviceptr d_indices;
	OptixTraversableHandle gas_hnd;
	CUdeviceptr d_gas_op_buffer;

	int32_t material_index;
} primitive;

primitive primitive_create(const cgltf_data* data, cgltf_primitive* curr_prim, const OptixDeviceContext ctx, const cudaStream_t stream);
void primitive_destroy(primitive p);
