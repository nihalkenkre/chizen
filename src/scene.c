#include "scene.h"
#include "utils.h"

#include <string.h>

CUdeviceptr d_images = 0;
CUdeviceptr d_ias_op_buffer = 0;

static size_t images_count = 0;
static size_t textures_count = 0;

scene scene_create_from_gltf(cgltf_data* gltf_data, const OptixProgramGroup ch_pg, const OptixModule module, const OptixDeviceContext ctx, const cudaStream_t stream)
{
	CHIZEN_RESULT chi_result = 0;
	cudaError_t cuda_error = 0;
	OptixResult optix_result = 0;

	scene s = { 0 };
	image* images = NULL;
	texture* textures = NULL;
	material* materials = NULL;
	light* lights = NULL;
	CUdeviceptr d_instances = 0;
	CUdeviceptr d_tmp_buffer = 0;

	if (gltf_data->cameras_count == 0)
	{
		printf("Please have at least one perspective camera in the scene.\nExiting...");
		s.result = CHIZEN_RESULT_PERSP_CAM_NOT_FOUND;
		goto cpu_error;
	}
	else
	{
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
			s.result = CHIZEN_RESULT_PERSP_CAM_NOT_FOUND;
			goto cpu_error;
		}
	}

	size_t images_size = sizeof(image) * gltf_data->images_count;
	images = calloc(1, images_size);
	if (images == NULL)
	{
		printf("calloc failed for images\n");
		s.result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
		goto cpu_error;
	}

	for (size_t i = 0; i < gltf_data->images_count; ++i)
	{
		images[i] = image_create(gltf_data, gltf_data->images + i);
		CHIZEN_RESULT_CHECK("scene image create", images[i].result, s.result);
	}

	CU_CHECK("alloc scene d_images", cudaMalloc((void**)&d_images, images_size), s.result);
	CU_CHECK("copy to scene d_images", cudaMemcpy((void*)d_images, images, images_size, cudaMemcpyHostToDevice), s.result);

	size_t texture_size = sizeof(texture) * gltf_data->textures_count;
	textures = calloc(1, texture_size);
	if (textures == NULL)
	{
		printf("calloc failed for textures\n");
		s.result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
		goto cpu_error;
	}

	for (size_t t = 0; t < gltf_data->textures_count; ++t)
	{
		textures[t] = texture_create(gltf_data, gltf_data->textures + t, images);
		CHIZEN_RESULT_CHECK("scene texture create", textures[t].result, s.result);
	}

	CU_CHECK("alloc scene d_textures", cudaMalloc((void**)&s.d_textures, texture_size), s.result);
	CU_CHECK("copy to scene d_textures", cudaMemcpy((void*)s.d_textures, textures, texture_size, cudaMemcpyHostToDevice), s.result);

	size_t materials_size = sizeof(material) * gltf_data->materials_count;
	materials = calloc(1, materials_size);
	if (materials == NULL)
	{
		printf("calloc failed for materials\n");
		s.result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
		goto cpu_error;
	}

	for (size_t m = 0; m < gltf_data->materials_count; ++m)
	{
		materials[m] = material_create(gltf_data, gltf_data->materials + m, textures);
		CHIZEN_RESULT_CHECK("scene material create", materials[m].result, s.result);
	}

	CU_CHECK("alloc scene d_materials", cudaMalloc((void**)&s.d_materials, materials_size), s.result);
	CU_CHECK("copy to scene d_materials", cudaMemcpy((void*)s.d_materials, materials, materials_size, cudaMemcpyHostToDevice), s.result);

	size_t lights_size = sizeof(light) * gltf_data->lights_count;
	lights = calloc(1, lights_size);

	if (lights == NULL)
	{
		printf("calloc failed for lights\n");
		s.result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
		goto cpu_error;
	}

	for (size_t l = 0; l < gltf_data->lights_count; ++l)
	{
		lights[l] = light_create(gltf_data, gltf_data->lights + l);
		CHIZEN_RESULT_CHECK("scene light create", lights[l].result, s.result);
	}

	size_t meshes_size = sizeof(mesh) * gltf_data->meshes_count;
	s.meshes_count = gltf_data->meshes_count;
	s.meshes = calloc(1, meshes_size);

	if (s.meshes == NULL)
	{
		printf("calloc failed for meshes\n");
		s.result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
		goto cpu_error;
	}

	for (size_t m = 0; m < gltf_data->meshes_count; ++m)
	{
		*(s.meshes + m) = mesh_create(gltf_data, gltf_data->meshes + m, ch_pg, module, ctx, stream, &s.ch_infos);
		CHIZEN_RESULT_CHECK("scene mesh create", (s.meshes + m)->result, s.result);
	}

	s.instances = instances_create(gltf_data, s.meshes);
	size_t d_instances_size = sizeof(OptixInstance) * s.instances.count;

	CU_CHECK("alloc scene d_instances", cudaMalloc((void**)&d_instances, d_instances_size), s.result);
	CU_CHECK("copy to scene d_instances", cudaMemcpy((void*)d_instances, s.instances.instances, d_instances_size, cudaMemcpyHostToDevice), s.result);

	const OptixBuildInput build_input = {
		.type = OPTIX_BUILD_INPUT_TYPE_INSTANCES,
		.instanceArray = {
			.instances = d_instances,
			.numInstances = (unsigned int)s.instances.count,
		},
	};

	const OptixAccelBuildOptions accel_options = {
		.operation = OPTIX_BUILD_OPERATION_BUILD,
		.buildFlags = OPTIX_BUILD_FLAG_PREFER_FAST_TRACE,
	};

	OptixAccelBufferSizes buffer_sizes = { 0 };
	OPTIX_CHECK("compute mem usage", optixAccelComputeMemoryUsage(ctx, &accel_options, &build_input, 1, &buffer_sizes), s.result);

	CU_CHECK("alloc ias tmp buffer", cudaMalloc((void**)&d_tmp_buffer, buffer_sizes.tempSizeInBytes), s.result);
	CU_CHECK("alloc ias op buffer", cudaMalloc((void**)&d_ias_op_buffer, buffer_sizes.outputSizeInBytes), s.result);

	CU_CHECK("scene stream sync", cudaStreamSynchronize(stream), s.result);
	OPTIX_CHECK("build scene ias", optixAccelBuild(ctx, stream, &accel_options, &build_input, 1, d_tmp_buffer, buffer_sizes.tempSizeInBytes, d_ias_op_buffer, buffer_sizes.outputSizeInBytes, &s.ias_hnd, NULL, 0), s.result);

	CU_CHECK("scene stream sync", cudaStreamSynchronize(stream), s.result);

cpu_error:
	CU_CHECK("free scene d_instancse", cudaFree((void*)d_instances), s.result);
	CU_CHECK("free scene tmp buffer", cudaFree((void*)d_tmp_buffer), s.result);

gpu_error:
	free(images);
	free(textures);
	free(materials);
	free(lights);

	return s;
}

CHIZEN_RESULT scene_destroy(scene s)
{
	CHIZEN_RESULT chi_result = CHIZEN_RESULT_SUCCESS;
	cudaError_t cuda_error = 0;
	OptixResult optix_result = 0;

	for (size_t i = 0; i < images_count; ++i)
	{
		image img = { 0 };
		CU_CHECK("copy d_image to host", cudaMemcpy((void*)&img, (void*)((size_t)d_images + (sizeof(image) * i)), sizeof(image), cudaMemcpyDeviceToHost), s.result);
		image_destroy(img);
	}

	for (size_t t = 0; t < textures_count; ++t)
	{
		texture tex = { 0 };
		CU_CHECK("copy d_texture to host", cudaMemcpy((void*)&tex, (void*)((size_t)s.d_textures + (sizeof(texture) * t)), sizeof(texture), cudaMemcpyDeviceToHost), s.result);
		texture_destroy(tex);
	}

	CU_CHECK("free scene d_images",cudaFree((void*)d_images), chi_result);
	CU_CHECK("free scene d_textures",cudaFree((void*)s.d_textures), chi_result);
	CU_CHECK("free scene d_materials",cudaFree((void*)s.d_materials), chi_result);
	CU_CHECK("free scene d_lights",cudaFree((void*)s.d_lights), chi_result);
	CU_CHECK("free scene d_ias_op_buffer",cudaFree((void*)d_ias_op_buffer), chi_result);

gpu_error:
	instances_destroy(s.instances);
	free(s.ch_infos.ch_records);
	s.ch_infos.ch_records = NULL;
	s.ch_infos.count = 0;

	free(s.meshes);
	s.meshes = NULL;
	s.meshes_count = 0;

	return chi_result;
}
