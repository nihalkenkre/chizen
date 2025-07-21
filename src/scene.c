#include "scene.h"
#include "utils.h"

#include <string.h>

scene scene_create(const char* gltf_path, const OptixDeviceContext ctx, const cudaStream_t stream)
{
	scene s = { 0 };
	cgltf_options gltf_options = { 0 };
	cgltf_data* gltf_data = NULL;

	if (cgltf_parse_file(&gltf_options, gltf_path, &gltf_data) != cgltf_result_success ||
		cgltf_validate(gltf_data) != cgltf_result_success ||
		cgltf_load_buffers(&gltf_options, gltf_data, gltf_path) != cgltf_result_success)
	{
		printf("Error parsing %s\n", gltf_path);
		goto shutdown;
	}

	for (size_t n = 0; n < gltf_data->nodes_count; ++n)
	{
		cgltf_node* curr_node = gltf_data->nodes + n;
		if (curr_node->mesh != NULL)
		{
			cgltf_mesh* curr_mesh = curr_node->mesh;

			if (s.meshes_count == 0)
			{
				s.meshes = malloc(sizeof(mesh));
			}
			else
			{
				s.meshes = realloc(s.meshes, sizeof(mesh) * (s.meshes_count + 1));
			}

			s.meshes[s.meshes_count++] = mesh_create(gltf_data, curr_node, ctx, stream);
		}
		else if (curr_node->camera != NULL)
		{
			if (curr_node->camera->type == cgltf_camera_type_perspective)
			{
				s.camera = camera_create(curr_node);
			}
		}
	}

	size_t instances_count = 0;

	for (size_t m = 0; m < s.meshes_count; ++m)
	{
		instances_count += s.meshes[m].prims_count;
	}

	size_t instances_size = sizeof(OptixInstance) * instances_count;
	OptixInstance* instances = malloc(instances_size);
	memset(instances, 0, instances_size);
	size_t instance_idx = 0;

	for (size_t m = 0; m < s.meshes_count; ++m)
	{
		for (size_t p = 0; p < s.meshes[m].prims_count; ++p)
		{
			instances[instance_idx++] = s.meshes[m].instances[p];
		}
	}

	CUdeviceptr d_instances = 0;
	CU_CHECK("alloc d_instances", cudaMalloc((void**)&d_instances, instances_size));
	CU_CHECK("copy instances to device", cudaMemcpy((void*)d_instances, instances, instances_size, cudaMemcpyHostToDevice));

	const OptixBuildInput build_input = {
		.type = OPTIX_BUILD_INPUT_TYPE_INSTANCES,
		.instanceArray = {
			.instances = d_instances,
			.numInstances = (unsigned int)instances_count,
		},
	};

	const OptixAccelBuildOptions accel_options = {
		.operation = OPTIX_BUILD_OPERATION_BUILD,
		.buildFlags = OPTIX_BUILD_FLAG_PREFER_FAST_TRACE | OPTIX_BUILD_FLAG_ALLOW_COMPACTION,
	};

	OptixAccelBufferSizes buffer_sizes = { 0 };
	OPTIX_CHECK("compute ias accel sizes", optixAccelComputeMemoryUsage(ctx, &accel_options, &build_input, 1, &buffer_sizes));

	CUdeviceptr d_tmp_buffer = 0;
	CU_CHECK("alloc ias d_tmp_buffer", cudaMalloc((void**)&d_tmp_buffer, buffer_sizes.tempSizeInBytes));
	CU_CHECK("alloc ias d_ias_buffer", cudaMalloc((void**)&s.d_ias_op_buffer, buffer_sizes.outputSizeInBytes));

	OPTIX_CHECK("ias accel build", optixAccelBuild(ctx, stream, &accel_options, &build_input, 1, d_tmp_buffer, buffer_sizes.tempSizeInBytes, s.d_ias_op_buffer, buffer_sizes.outputSizeInBytes, &s.ias_hnd, NULL, 0));

	CU_CHECK("ias accel build sync", cudaStreamSynchronize(stream));
	CU_CHECK("dealloc d_instances", cudaFree((void*)d_instances));
	CU_CHECK("dealloc d_tmp_buffer", cudaFree((void*)d_tmp_buffer));

	free(instances);

shutdown:
	cgltf_free(gltf_data);

	return s;
}

void scene_destroy(scene s)
{
	if (s.meshes != NULL)
	{
		for (size_t m = 0; m < s.meshes_count; ++m)
		{
			mesh_destroy(s.meshes[m]);
		}

		free(s.meshes);
		s.meshes_count = 0;
	}

	CU_CHECK("dealloc ias accel buffer", cudaFree((void*)s.d_ias_op_buffer));
}
