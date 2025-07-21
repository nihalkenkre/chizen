#include "primitive.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>

primitive primitive_create(cgltf_data* data, cgltf_primitive* curr_prim, const OptixDeviceContext ctx, const cudaStream_t stream)
{
	primitive p = { 0 };

	p.indices_count = curr_prim->indices->count;
	p.indices = malloc(curr_prim->indices->count * sizeof(uint32_t));

	if (curr_prim->indices->component_type == cgltf_component_type_r_32u)
	{
		memcpy(p.indices, (void*)((size_t)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->buffer_view->offset + curr_prim->indices->offset), p.indices_count * sizeof(uint32_t));
	}
	else if (curr_prim->indices->component_type == cgltf_component_type_r_16u)
	{
		uint16_t* idxs = (uint16_t*)((size_t)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->buffer_view->offset + curr_prim->indices->offset);

		for (size_t i = 0; i < p.indices_count; ++i)
		{
			p.indices[i] = idxs[i];
		}
	}

	for (size_t a = 0; a < curr_prim->attributes_count; ++a)
	{
		cgltf_attribute* curr_attr = curr_prim->attributes + a;

		if (strcmp(curr_attr->name, "POSITION") == 0)
		{
			p.positions_count = curr_attr->data->count;
			p.positions = malloc(curr_attr->data->buffer_view->size);
			memcpy(p.positions, (void*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->buffer_view->size);
		}
		else if (strcmp(curr_attr->name, "NORMAL") == 0)
		{
			p.normals_count = curr_attr->data->count;
			p.normals = malloc(curr_attr->data->buffer_view->size);
			memcpy(p.normals, (void*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->buffer_view->size);
		}
		else if (strcmp(curr_attr->name, "TEXCOORD_0") == 0)
		{
			p.uvs_count = curr_attr->data->count;
			p.uvs = malloc(curr_attr->data->buffer_view->size);
			memcpy(p.uvs, (void*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->buffer_view->size);
		}
	}

	if (curr_prim->material != NULL)
	{
		p.material_index = (int32_t)cgltf_material_index(data, curr_prim->material);
	}
	else
	{
		p.material_index = -1;
	}

	size_t positions_size = p.positions_count * sizeof(vec3);
	CU_CHECK("alloc prim positions", cudaMalloc((void**)&p.d_positions, positions_size));
	CU_CHECK("copy positions to device", cudaMemcpy((void*)p.d_positions, p.positions, positions_size, cudaMemcpyHostToDevice));

	size_t normals_size = p.normals_count * sizeof(vec3);
	CU_CHECK("alloc prim normals", cudaMalloc((void**)&p.d_normals, normals_size));
	CU_CHECK("copy normals to device", cudaMemcpy((void*)p.d_normals, p.normals, normals_size, cudaMemcpyHostToDevice));

	size_t uvs_size = p.uvs_count * sizeof(vec2);
	if (uvs_size > 0)
	{
		CU_CHECK("alloc prim uvss", cudaMalloc((void**)&p.d_uvs, uvs_size));
		CU_CHECK("copy uvss to device", cudaMemcpy((void*)p.d_uvs, p.uvs, uvs_size, cudaMemcpyHostToDevice));
	}

	size_t indices_size = p.indices_count * sizeof(uint32_t);
	CU_CHECK("alloc prim indices", cudaMalloc((void**)&p.d_indices, indices_size));
	CU_CHECK("copy indices to device", cudaMemcpy((void*)p.d_indices, p.indices, indices_size, cudaMemcpyHostToDevice));

	const unsigned int flags[1] = { 0 };

	const OptixBuildInput build_input = {
		.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES,
		.triangleArray = {
			.vertexBuffers = &p.d_positions,
			.numVertices = (unsigned int)p.positions_count,
			.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3,
			.indexBuffer = p.d_indices,
			.numIndexTriplets = (unsigned int)(p.indices_count / 3),
			.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3,
			.flags = flags,
			.numSbtRecords = 1,
		},
	};

	const OptixAccelBuildOptions build_options = {
		.buildFlags = OPTIX_BUILD_FLAG_PREFER_FAST_TRACE | OPTIX_BUILD_FLAG_ALLOW_COMPACTION,
		.operation = OPTIX_BUILD_OPERATION_BUILD,
	};

	OptixAccelBufferSizes buffer_sizes = { 0 };
	OPTIX_CHECK("compute gas accel build sizes", optixAccelComputeMemoryUsage(ctx, &build_options, &build_input, 1, &buffer_sizes));

	CUdeviceptr tmp_buffer = 0;
	CU_CHECK("alloc gas tmp_buffer", cudaMalloc((void**)&tmp_buffer, buffer_sizes.tempSizeInBytes));
	CU_CHECK("alloc accel op_buffer", cudaMalloc((void**)&p.d_gas_op_buffer, buffer_sizes.outputSizeInBytes + sizeof(custom_gas_data)));

	const custom_gas_data cgd = {
		.normals = (float3*)p.d_normals,
		.uvs = (float2*)p.d_uvs,
		.indices = (uint3*)p.d_indices,
	};
	CU_CHECK("copy deadbeefcafebabe", cudaMemcpy((void*)p.d_gas_op_buffer, &cgd, sizeof(custom_gas_data), cudaMemcpyHostToDevice));

	OPTIX_CHECK("gas accel build", optixAccelBuild(ctx, stream, &build_options, &build_input, 1, tmp_buffer, buffer_sizes.tempSizeInBytes, p.d_gas_op_buffer + sizeof(custom_gas_data), buffer_sizes.outputSizeInBytes, &p.gas_hnd, NULL, 0));
	CU_CHECK("sync gas accel build", cudaStreamSynchronize(stream));

	CU_CHECK("dealloc gas tmp_buffer", cudaFree((void*)tmp_buffer));

	return p;
}

void primitive_destroy(primitive p)
{
	if (p.positions != NULL)
	{
		free(p.positions);
		p.positions = NULL;
		p.positions_count = 0;
	}

	if (p.normals != NULL)
	{
		free(p.normals);
		p.normals = NULL;
		p.normals_count = 0;
	}

	if (p.uvs != NULL)
	{
		free(p.uvs);
		p.uvs = NULL;
		p.uvs_count = 0;
	}

	if (p.indices != NULL)
	{
		free(p.indices);
		p.indices = NULL;
		p.indices_count = 0;
	}

	CU_CHECK("dealloc d_positions", cudaFree((void*)p.d_positions));
	CU_CHECK("dealloc d_normals", cudaFree((void*)p.d_normals));
	CU_CHECK("dealloc d_uvs", cudaFree((void*)p.d_uvs));
	CU_CHECK("dealloc d_indices", cudaFree((void*)p.d_indices));
	CU_CHECK("dealloc d_gas_op_buffer", cudaFree((void*)p.d_gas_op_buffer));
}
