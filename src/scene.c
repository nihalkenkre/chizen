#include "scene.h"
#include "utils.h"

#include <string.h>

CUdeviceptr d_images = 0;
CUdeviceptr d_ias_op_buffer = 0;

static size_t images_count = 0;
static size_t textures_count = 0;

scene scene_create_from_gltf(cgltf_data* gltf_data, const OptixDeviceContext ctx, const cudaStream_t stream)
{
	scene s = { 0 };

	if (gltf_data->cameras_count == 0)
	{
		printf("Please have at least one perspective camera in the scene.\nExiting...");
		exit(0xdeadbabe);
	}
	else {
		bool is_persp_cam_found = false;
		for (size_t n = 0; n < gltf_data->nodes_count; ++n)
		{
			if (gltf_data->nodes[n].camera == NULL)
				continue;

			cgltf_node* curr_node = gltf_data->nodes + n;
			if (curr_node->camera->type == cgltf_camera_type_perspective)
			{
				s.camera = camera_create_from_gltf(curr_node);
				is_persp_cam_found = true;
				break;
			}
		}

		if (!is_persp_cam_found)
		{
			printf("Please have at least one perspective camera in the scene.\nExiting...");
			exit(0xdeadbabe);
		}
	}

	size_t images_size = sizeof(image) * gltf_data->images_count;
	image* images = malloc(images_size);
	for (size_t i = 0; i < gltf_data->images_count; ++i)
	{
		images[i] = image_create(gltf_data, gltf_data->images + i);
	}

	CU_CHECK("alloc d_textures", cudaMalloc((void**)&d_images, images_size));
	CU_CHECK("copy d_textures to device", cudaMemcpy((void*)d_images, images, images_size, cudaMemcpyHostToDevice));

	size_t texture_size = sizeof(texture) * gltf_data->textures_count;
	texture* textures = malloc(texture_size);
	for (size_t t = 0; t < gltf_data->textures_count; ++t)
	{
		textures[t] = texture_create(gltf_data, gltf_data->textures + t, images);
	}

	CU_CHECK("alloc d_textures", cudaMalloc((void**)&s.d_textures, texture_size));
	CU_CHECK("copy d_textures to device", cudaMemcpy((void*)s.d_textures, textures, texture_size, cudaMemcpyHostToDevice));

	size_t materials_size = sizeof(material) * gltf_data->materials_count;
	material* materials = malloc(materials_size);

	for (size_t m = 0; m < gltf_data->materials_count; ++m)
	{
		materials[m] = material_create(gltf_data, gltf_data->materials + m, textures);
	}

	CU_CHECK("alloc d_materials", cudaMalloc((void**)&s.d_materials, materials_size));
	CU_CHECK("copy d_materials to device", cudaMemcpy((void*)s.d_materials, materials, materials_size, cudaMemcpyHostToDevice));

	size_t meshes_size = sizeof(mesh) * gltf_data->meshes_count;
	mesh* meshes = malloc(meshes_size);

	for (size_t m = 0; m < gltf_data->meshes_count; ++m)
	{
		meshes[m] = mesh_create(gltf_data, gltf_data->meshes + m, ctx, stream);
	}

	size_t instances_count = 0;

	for (size_t n = 0; n < gltf_data->nodes_count; ++n)
	{
		if (gltf_data->nodes[n].mesh == NULL)
			continue;

		instances_count += meshes[cgltf_mesh_index(gltf_data, gltf_data->nodes[n].mesh)].prims_count;
	}

	size_t instances_size = sizeof(OptixInstance) * instances_count;
	OptixInstance* instances = malloc(instances_size);
	memset(instances, 0, instances_size);

	size_t instance_idx = 0;

	for (size_t n = 0; n < gltf_data->nodes_count; ++n)
	{
		if (gltf_data->nodes[n].mesh == NULL)
			continue;

		mat4 xform = { 0 };
		utils_get_xform_matrix_for_node(gltf_data->nodes + n, xform);
		mat4 xform_xposed = { 0 };
		glm_mat4_transpose_to(xform, xform_xposed);

		size_t mesh_idx = cgltf_mesh_index(gltf_data, gltf_data->nodes[n].mesh);

		for (size_t p = 0; p < meshes[mesh_idx].prims_count; ++p)
		{
			instances[instance_idx].flags = 0;
			instances[instance_idx].visibilityMask = 0xFF;
			instances[instance_idx].traversableHandle = meshes[mesh_idx].prims[p].gas_hnd;
			memcpy(instances[instance_idx].transform, xform_xposed, sizeof(float) * 12);

			++instance_idx;
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
	CU_CHECK("alloc ias d_ias_buffer", cudaMalloc((void**)&d_ias_op_buffer, buffer_sizes.outputSizeInBytes));

	OPTIX_CHECK("ias accel build", optixAccelBuild(ctx, stream, &accel_options, &build_input, 1, d_tmp_buffer, buffer_sizes.tempSizeInBytes, d_ias_op_buffer, buffer_sizes.outputSizeInBytes, &s.ias_hnd, NULL, 0));

	CU_CHECK("ias accel build sync", cudaStreamSynchronize(stream));
	CU_CHECK("dealloc d_instances", cudaFree((void*)d_instances));
	CU_CHECK("dealloc d_tmp_buffer", cudaFree((void*)d_tmp_buffer));

	free(instances);
	free(images);
	free(textures);
	free(materials);
	free(meshes);

	return s;
}

void scene_destroy(scene s)
{
	for (size_t i = 0; i < images_count; ++i)
	{
		image img = { 0 };
		CU_CHECK("copy image to host", cudaMemcpy((void*)&img, (void*)((size_t)d_images + (sizeof(image) * i)), sizeof(image), cudaMemcpyDeviceToHost));
		image_destroy(img);
	}
	CU_CHECK("dealloc d_images", cudaFree((void*)d_images));

	for (size_t t = 0; t < textures_count; ++t)
	{
		texture tex = { 0 };
		CU_CHECK("copy texture to host", cudaMemcpy((void*)&tex, (void*)((size_t)s.d_textures + (sizeof(texture) * t)), sizeof(texture), cudaMemcpyDeviceToHost));
		texture_destroy(tex);
	}
	CU_CHECK("dealloc d_textures", cudaFree((void*)s.d_textures));
	CU_CHECK("dealloc d_materials", cudaFree((void*)s.d_materials));

	CU_CHECK("dealloc ias accel buffer", cudaFree((void*)d_ias_op_buffer));
}
