#include "mesh_optix.h"
#include "../common/utils.h"
#include "utils.h"

#include <string.h>

void create_gas_hnd(mesh_optix* m, const OptixDeviceContext ctx, const cudaStream_t stream)
{

}

mesh_optix mesh_optix_create(cgltf_node* curr_node, const OptixDeviceContext ctx, const cudaStream_t stream)
{
	mesh_optix m = { 0 };

	utils_get_xform_matrix_for_node(curr_node, m.xform);

	cgltf_mesh* curr_mesh = curr_node->mesh;

	m.prims_count = curr_mesh->primitives_count;
	m.prims = malloc(sizeof(primitive_optix) * m.prims_count);

	CUdeviceptr* d_positions = malloc(sizeof(CUdeviceptr) * m.prims_count);
	memset(d_positions, 0, sizeof(CUdeviceptr) * m.prims_count);
	CUdeviceptr* d_indices = malloc(sizeof(CUdeviceptr) * m.prims_count);
	memset(d_indices, 0, sizeof(CUdeviceptr) * m.prims_count);
	OptixBuildInput* build_inputs = malloc(sizeof(OptixBuildInput) * m.prims_count);
	memset(build_inputs, 0, sizeof(OptixBuildInput) * m.prims_count);

	unsigned int gas_flags[1] = { OPTIX_BUILD_FLAG_NONE };
	for (size_t p = 0; p < m.prims_count; ++p)
	{
		m.prims[p] = primitive_optix_create(curr_mesh->primitives + p, m.xform, ctx, stream);

		size_t positions_size = m.prims[p].positions_count * sizeof(vec3);
		CU_CHECK("alloc prim positions", cudaMalloc((void**)&d_positions[p], positions_size));
		CU_CHECK("copy positions to device", cudaMemcpy((void*)d_positions[p], m.prims[p].positions, positions_size, cudaMemcpyHostToDevice));

		size_t indices_size = m.prims[p].indices_count * sizeof(uint32_t);
		CU_CHECK("alloc prim indices", cudaMalloc((void**)&d_indices[p], indices_size));
		CU_CHECK("copy indices to device", cudaMemcpy((void*)d_indices[p], m.prims[p].indices, indices_size, cudaMemcpyHostToDevice));

		build_inputs[p].type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
		build_inputs[p].triangleArray.numVertices = (unsigned int)(m.prims[p].positions_count);
		build_inputs[p].triangleArray.vertexBuffers = &d_positions[p];
		build_inputs[p].triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
		build_inputs[p].triangleArray.vertexStrideInBytes = 0;
		build_inputs[p].triangleArray.indexBuffer = d_indices[p];
		build_inputs[p].triangleArray.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
		build_inputs[p].triangleArray.indexStrideInBytes = 0;
		build_inputs[p].triangleArray.numIndexTriplets = (unsigned int)(m.prims[p].indices_count / 3);
		build_inputs[p].triangleArray.numSbtRecords = 1;
		build_inputs[p].triangleArray.flags = gas_flags;
	}

	const OptixAccelBuildOptions build_options = {
		.operation = OPTIX_BUILD_OPERATION_BUILD,
		.buildFlags = OPTIX_BUILD_FLAG_PREFER_FAST_TRACE | OPTIX_BUILD_FLAG_ALLOW_COMPACTION,
	};

	OptixAccelBufferSizes buffer_sizes = { 0 };
	OPTIX_CHECK("compute gas accel build sizes", optixAccelComputeMemoryUsage(ctx, &build_options, build_inputs, (unsigned int)m.prims_count, &buffer_sizes));

	CUdeviceptr tmp_buffer = 0;
	OPTIX_CHECK("alloc gas tmp_buffer", cudaMalloc((void**)&tmp_buffer, buffer_sizes.tempSizeInBytes));
	OPTIX_CHECK("alloc accel op_buffer", cudaMalloc((void**)&m.accel_gas_buffer, buffer_sizes.outputSizeInBytes));

	OPTIX_CHECK("gas accel build", optixAccelBuild(ctx, stream, &build_options, build_inputs, (unsigned int)m.prims_count, tmp_buffer, buffer_sizes.tempSizeInBytes, m.accel_gas_buffer, buffer_sizes.outputSizeInBytes, &m.gas_hnd, NULL, 0));
	CU_CHECK("sync gas accel build", cudaStreamSynchronize(stream));

	CU_CHECK("dealloc gas tmp_buffer", cudaFree((void*)tmp_buffer));

	unsigned int inst_flags = OPTIX_BUILD_FLAG_NONE;

	m.instance.flags = inst_flags;
	m.instance.visibilityMask = 0xFF;
	m.instance.traversableHandle = m.gas_hnd;

	mat4 xform_transposed = { 0 };
	glm_mat4_transpose_to(m.xform, xform_transposed);
	memcpy(m.instance.transform, xform_transposed, sizeof(float) * 12);

	for (size_t p = 0; p < m.prims_count; ++p)
	{
		CU_CHECK("dealloc prim positions", cudaFree((void*)d_positions[p]));
		CU_CHECK("dealloc prim indices", cudaFree((void*)d_indices[p]));
	}

	free(build_inputs);
	free(d_indices);
	free(d_positions);

	return m;
}

void mesh_optix_destroy(mesh_optix m)
{
	if (m.prims != NULL)
	{
		for (size_t p = 0; p < m.prims_count; ++p)
		{
			primitive_optix_destroy(m.prims[p]);
		}

		free(m.prims);
		m.prims = NULL;
		m.prims_count = 0;
	}

	CU_CHECK("dealloc accel gas buffer", cudaFree((void*)m.accel_gas_buffer));
}
