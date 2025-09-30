#include "mesh.h"
#include "utils.h"

#include <string.h>

mesh mesh_create(const cgltf_data* data, cgltf_mesh* curr_mesh, const OptixProgramGroup ch_od_pg, const OptixProgramGroup ch_ld_pg, const OptixDeviceContext ctx, const cudaStream_t stream, ch_infos* ch_infos)
{
	CHIZEN_RESULT chi_result = 0;
	cudaError_t cuda_error = 0;
	OptixResult optix_result = 0;

	mesh m = { 0 };
	OptixBuildInput* build_inputs = NULL;
	CUdeviceptr tmp_buffer = 0;

	m.prims_count = curr_mesh->primitives_count;
	m.prims = calloc(1, sizeof(primitive) * m.prims_count);
	if (m.prims == NULL)
	{
		printf("calloc failed for m.prims\n");
		m.result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
		goto cpu_error;
	}

	build_inputs = calloc(1, sizeof(OptixBuildInput) * m.prims_count);
	if (build_inputs == NULL)
	{
		printf("calloc failed for mesh build inputs\n");
		m.result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
		goto cpu_error;
	}

	const unsigned int flags[1] = { 0 };

	for (size_t p = 0; p < m.prims_count; ++p)
	{
		m.prims[p] = primitive_create(data, curr_mesh->primitives + p, ch_od_pg, ch_ld_pg, ctx, stream, ch_infos);
		CHIZEN_RESULT_CHECK("primitive create", m.prims[p].result, m.result);

		build_inputs[p] = m.prims[p].build_input;
		build_inputs[p].triangleArray.flags = flags;
		build_inputs[p].triangleArray.vertexBuffers = &m.prims[p].d_positions;
	}

	const OptixAccelBuildOptions accel_options = {
		.buildFlags = OPTIX_BUILD_FLAG_PREFER_FAST_TRACE | OPTIX_BUILD_FLAG_ALLOW_COMPACTION,
		.operation = OPTIX_BUILD_OPERATION_BUILD,
	};

	OptixAccelBufferSizes buffer_sizes = { 0 };
	OPTIX_CHECK("compute prim memory usage", optixAccelComputeMemoryUsage(ctx, &accel_options, build_inputs, (unsigned int)m.prims_count, &buffer_sizes), m.result);

	CU_CHECK("alloc prim tmp buffer", cudaMalloc((void**)&tmp_buffer, buffer_sizes.tempSizeInBytes), m.result);
	CU_CHECK("alloc prim op buffer", cudaMalloc((void**)&m.d_op_gas_buffer, buffer_sizes.outputSizeInBytes), m.result);

	OPTIX_CHECK("prim accel build", optixAccelBuild(ctx, stream, &accel_options, build_inputs, (unsigned int)m.prims_count, tmp_buffer, buffer_sizes.tempSizeInBytes, m.d_op_gas_buffer, buffer_sizes.outputSizeInBytes, &m.gas_hnd, NULL, 0), m.result);
	CU_CHECK("prim stream sync", cudaStreamSynchronize(stream), m.result);

cpu_error:
	CU_CHECK("free tmp buffer", cudaFree((void*)tmp_buffer), m.result);

gpu_error:
	free(build_inputs);
	build_inputs = NULL;

	return m;
}

CHIZEN_RESULT mesh_destroy(mesh m)
{
	CHIZEN_RESULT chi_result = CHIZEN_RESULT_SUCCESS;
	cudaError_t cuda_error = cudaSuccess;

	for (size_t p = 0; p < m.prims_count; ++p)
	{
		primitive_destroy(m.prims[p]);
	}

	free(m.prims);
	m.prims = NULL;
	m.prims_count = 0;

	CU_CHECK("free mesh op gas buffer", cudaFree((void*)m.d_op_gas_buffer), chi_result);

gpu_error:
	return chi_result;
}
