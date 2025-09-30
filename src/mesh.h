#pragma once

#include "primitive.h"
#include <cglm/include/cglm/cglm.h>
#include "error.h"

typedef struct mesh
{
	primitive* prims;
	size_t prims_count;
	OptixTraversableHandle gas_hnd;
	CUdeviceptr d_op_gas_buffer;
	CHIZEN_RESULT result;
} mesh;

mesh mesh_create(const cgltf_data* data, cgltf_mesh* curr_mesh, const OptixProgramGroup ch_od_pg, const OptixProgramGroup ch_ld_pg, const OptixDeviceContext ctx, const cudaStream_t stream, ch_infos* ch_infos);
CHIZEN_RESULT mesh_destroy(mesh m);