#include "primitive.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>

CHIZEN_RESULT create_ch_records(const cgltf_data* gltf_data, cgltf_primitive* curr_prim, ch_infos* ch_infos)
{
	CHIZEN_RESULT chi_result = CHIZEN_RESULT_SUCCESS;

	return chi_result;
}

primitive primitive_create(const cgltf_data* gltf_data, cgltf_primitive* curr_prim, const OptixProgramGroup ch_od_pg, const OptixProgramGroup ch_ld_pg, const OptixDeviceContext ctx, const cudaStream_t stream, ch_infos* ch_infos)
{
	cudaError_t cuda_error = 0;
	OptixResult optix_result = 0;

	primitive p = { 0 };

	CUdeviceptr tmp_buffer = 0;
	size_t positions_count = 0;

	for (size_t a = 0; a < curr_prim->attributes_count; ++a)
	{
		cgltf_attribute* curr_attr = curr_prim->attributes + a;

		if (strcmp(curr_attr->name, "POSITION") == 0)
		{
			positions_count = curr_attr->data->count;
			size_t positions_size = curr_attr->data->buffer_view->size;
			CU_CHECK("alloc prim d_positions", cudaMalloc((void**)&p.d_positions, positions_size), p.result);
			CU_CHECK("copy to prim d_positions", cudaMemcpy((void*)p.d_positions,
				(void*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset),
				positions_size, cudaMemcpyHostToDevice), p.result);
		}
		else if (strcmp(curr_attr->name, "NORMAL") == 0)
		{
			size_t normals_size = curr_attr->data->buffer_view->size;
			CU_CHECK("alloc prim d_normals", cudaMalloc((void**)&p.d_normals, normals_size), p.result);
			CU_CHECK("copy to prim d_normals", cudaMemcpy((void*)p.d_normals,
				(void*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset),
				normals_size, cudaMemcpyHostToDevice), p.result);
		}
		else if (strcmp(curr_attr->name, "TEXCOORD_0") == 0)
		{
			size_t uvs_size = curr_attr->data->buffer_view->size;
			if (uvs_size > 0)
			{
				CU_CHECK("alloc prim d_uvs", cudaMalloc((void**)&p.d_uvs, uvs_size), p.result);
				CU_CHECK("copy to prim d_uvs", cudaMemcpy((void*)p.d_uvs,
					(void*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset),
					uvs_size, cudaMemcpyHostToDevice), p.result);
			}
		}
	}

	size_t indices_size = curr_prim->indices->buffer_view->size;
	OptixIndicesFormat indices_format = OPTIX_INDICES_FORMAT_NONE;

	if (curr_prim->indices->component_type == cgltf_component_type_r_16u)
	{
		indices_format = OPTIX_INDICES_FORMAT_UNSIGNED_SHORT3;
	}
	else if (curr_prim->indices->component_type == cgltf_component_type_r_32u)
	{
		indices_format = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
	}

	CU_CHECK("alloc prim d_indices", cudaMalloc((void**)&p.d_indices, indices_size), p.result);
	CU_CHECK("copy to prim d_indices", cudaMemcpy((void*)p.d_indices,
		(void*)((size_t)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->buffer_view->offset + curr_prim->indices->offset),
		indices_size, cudaMemcpyHostToDevice), p.result);

	p.build_input.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
	p.build_input.triangleArray.numVertices = (unsigned int)positions_count;
	p.build_input.triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
	p.build_input.triangleArray.indexBuffer = p.d_indices;
	p.build_input.triangleArray.numIndexTriplets = (unsigned int)(curr_prim->indices->count / 3);
	p.build_input.triangleArray.indexFormat = indices_format;
	p.build_input.triangleArray.numSbtRecords = 1;

	if (ch_infos->count == 0)
	{
		ch_infos->ch_records = calloc(RAY_TYPE_MAX, sizeof(ch_record));
		if (ch_infos->ch_records == NULL)
		{
			printf("calloc failed for prim ch_records\n");
			p.result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
			goto cpu_error;
		}
		ch_infos->count = RAY_TYPE_MAX;
	}
	else
	{
		ch_infos->count += RAY_TYPE_MAX;

		void* tmp_ch_records = realloc(ch_infos->ch_records, sizeof(ch_record) * ch_infos->count);
		if (tmp_ch_records == NULL)
		{
			printf("realloc failed for prim ch_records\n");
			p.result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
			goto cpu_error;
		}
		ch_infos->ch_records = tmp_ch_records;
	}

	// od ch_record
	ch_record* curr_ch_record = ch_infos->ch_records + (ch_infos->count - RAY_TYPE_MAX);
	OPTIX_CHECK("record pack header", optixSbtRecordPackHeader(ch_od_pg, curr_ch_record->header), p.result);

	curr_ch_record->data.indices = (void*)p.d_indices;
	curr_ch_record->data.indices_format = indices_format;
	if (curr_prim->material != NULL)
	{
		curr_ch_record->data.material_index = (int32_t)cgltf_material_index(gltf_data, curr_prim->material);
	}
	else
	{
		curr_ch_record->data.material_index = -1;
	}
	curr_ch_record->data.normals = (float3*)p.d_normals;
	curr_ch_record->data.uvs = (float2*)p.d_uvs;

	// ld ch_record
	curr_ch_record = ch_infos->ch_records + (ch_infos->count - (RAY_TYPE_MAX - 1));
	OPTIX_CHECK("record pack header", optixSbtRecordPackHeader(ch_ld_pg, curr_ch_record->header), p.result);

	curr_ch_record->data.indices = (void*)p.d_indices;
	curr_ch_record->data.indices_format = indices_format;
	if (curr_prim->material != NULL)
	{
		curr_ch_record->data.material_index = (int32_t)cgltf_material_index(gltf_data, curr_prim->material);
	}
	else
	{
		curr_ch_record->data.material_index = -1;
	}
	curr_ch_record->data.normals = (float3*)p.d_normals;
	curr_ch_record->data.uvs = (float2*)p.d_uvs;

cpu_error:
	CU_CHECK("free primitive tmp buffer", cudaFree((void*)tmp_buffer), p.result);

gpu_error:
	return p;
}

CHIZEN_RESULT primitive_destroy(primitive p)
{
	CHIZEN_RESULT chi_result = CHIZEN_RESULT_SUCCESS;
	cudaError_t cuda_error = cudaSuccess;

	CU_CHECK("free prim d_positions", cudaFree((void*)p.d_positions), chi_result);
	CU_CHECK("free prim d_normals", cudaFree((void*)p.d_normals), chi_result);
	CU_CHECK("free prim d_uvs", cudaFree((void*)p.d_uvs), chi_result);
	CU_CHECK("free prim d_indices", cudaFree((void*)p.d_indices), chi_result);

gpu_error:
	return chi_result;
}
