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

	size_t positions_size = 0;
	size_t indices_size = 0;
	for (size_t p = 0; p < m.prims_count; ++p)
	{
		m.prims[p] = primitive_optix_create(curr_mesh->primitives + p, m.xform, ctx, stream);
		positions_size += (sizeof(vec3) * m.prims[p].positions_count);
		indices_size += (sizeof(uint32_t) * m.prims[p].indices_count);
	}

	char* positions = malloc(positions_size);
	char* indices = malloc(indices_size);

	size_t positions_index = 0;
	size_t indices_index = 0;

	for (size_t p = 0; p < m.prims_count; ++p)
	{
		memcpy(positions + positions_index, m.prims[p].positions, (sizeof(vec3) * m.prims[p].positions_count));
		positions_index += (sizeof(vec3) * m.prims[p].positions_count);

		memcpy(indices + indices_index, m.prims[p].indices, (sizeof(uint32_t) * m.prims[p].indices_count));
		indices_index += (sizeof(uint32_t) * m.prims[p].indices_count);
	}

	CUdeviceptr d_indices = 0;
	CU_CHECK("alloc d_indices", cudaMalloc((void**)&d_indices, indices_size));
	CU_CHECK("copy indices to device", cudaMemcpy((void*)d_indices, indices, indices_size, cudaMemcpyHostToDevice));

	CUdeviceptr d_positions = 0;
	CU_CHECK("alloc d_vertices", cudaMalloc((void**)&d_positions, positions_size));
	CU_CHECK("copy positions to device", cudaMemcpy((void*)d_positions, positions, positions_size, cudaMemcpyHostToDevice));

	unsigned int gas_flags[1] = { OPTIX_BUILD_FLAG_NONE };

	const OptixBuildInput build_input = {
		.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES,
		.triangleArray = {
			.numVertices = (unsigned int)(positions_size / sizeof(vec3)),
			.vertexBuffers = &d_positions,
			.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3,
			.vertexStrideInBytes = 0,
			.indexBuffer = d_indices,
			.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3,
			.indexStrideInBytes = 0,
			.numIndexTriplets = (unsigned int)((indices_size / sizeof(uint32_t)) / 3),
			.numSbtRecords = 1,
			.flags = gas_flags,
		},
	};

	const OptixAccelBuildOptions build_options = {
		.operation = OPTIX_BUILD_OPERATION_BUILD,
		.buildFlags = OPTIX_BUILD_FLAG_PREFER_FAST_TRACE,
	};

	OptixAccelBufferSizes buffer_sizes = { 0 };
	OPTIX_CHECK("compute gas accel build sizes", optixAccelComputeMemoryUsage(ctx, &build_options, &build_input, 1, &buffer_sizes));

	CUdeviceptr tmp_buffer = 0;
	OPTIX_CHECK("alloc gas tmp_buffer", cudaMalloc((void**)&tmp_buffer, buffer_sizes.tempSizeInBytes));
	OPTIX_CHECK("alloc accel op_buffer", cudaMalloc((void**)&m.accel_gas_buffer, buffer_sizes.outputSizeInBytes));

	OPTIX_CHECK("gas accel build", optixAccelBuild(ctx, stream, &build_options, &build_input, 1, tmp_buffer, buffer_sizes.tempSizeInBytes, m.accel_gas_buffer, buffer_sizes.outputSizeInBytes, &m.gas_hnd, NULL, 0));
	CU_CHECK("sync gas accel build", cudaStreamSynchronize(stream));

	CU_CHECK("dealloc gas tmp_buffer", cudaFree((void*)tmp_buffer));

	unsigned int inst_flags = OPTIX_BUILD_FLAG_NONE;

	m.instance.flags = inst_flags;
	m.instance.visibilityMask = 0xFF;
	m.instance.traversableHandle = m.gas_hnd;

	mat4 xform_transposed = { 0 };
	glm_mat4_transpose_to(m.xform, xform_transposed);
	memcpy(m.instance.transform, xform_transposed, sizeof(float) * 12);

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
