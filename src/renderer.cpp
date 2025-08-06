#include "renderer.h"
#include "utils.h"
#include "common.h"

#include <Shlwapi.h>

static void log_cb(unsigned int level, const char* tag, const char* message, void* cbdata)
{
	printf("%d - %s: %s\n", level, tag, message);
}

void renderer_render_gltf(const size_t render_width, const size_t render_height, const uint8_t num_samples, const char* gltf_path, exr_pass* passes, const size_t passes_count)
{
	cudaStream_t stream = nullptr;
	cgltf_options gltf_options = { };
	cgltf_data* gltf_data = nullptr;
	OptixDeviceContext ctx = nullptr;
	OptixDeviceContextOptions ctx_options = { };
	scene s = { };
	OptixPipelineCompileOptions pipeline_compile_options = {};
	OptixModuleCompileOptions module_compile_options = {};
	char curr_dir[MAX_PATH];
	OFSTRUCT open_file = { 0 };
	HANDLE h_file = nullptr;
	LARGE_INTEGER file_size = {};
	void* module_data = nullptr;
	OptixModule module = nullptr;
	OptixPipelineLinkOptions pipeline_link_options = {};
	OptixProgramGroupDesc ray_gen_program_group_desc = {};
	OptixProgramGroupDesc miss_program_group_desc = {};
	OptixProgramGroupDesc closest_hit_program_group_desc = {};
	OptixProgramGroupOptions module_program_group_options = { 0 };
	OptixProgramGroup ray_gen_program_group = nullptr;
	OptixProgramGroup closest_hit_program_group = nullptr;
	OptixProgramGroup miss_program_group = nullptr;
	vec3 pixel_00_loc = { 0 }, pixel_delta_u = { 0 }, pixel_delta_v = { 0 };
	ray_gen_record rg_record = {};
	CUdeviceptr d_rand_states = 0;
	char closest_hit_record_header[OPTIX_SBT_RECORD_HEADER_SIZE];
	char miss_record_header[OPTIX_SBT_RECORD_HEADER_SIZE];
	CUdeviceptr d_ray_gen_record = 0;
	CUdeviceptr d_closest_hit_record = 0;
	CUdeviceptr d_miss_record = 0;
	OptixShaderBindingTable sbt = {};
	exr_pass* d_exr_passes_staging = nullptr;
	CUdeviceptr d_exr_passes = 0;
	launch_params lp = {};
	CUdeviceptr d_launch_params = 0;
	OptixPipeline pipeline = nullptr;
	OptixProgramGroup* pipeline_program_groups = nullptr;

	CU_CHECK(cudaFree(nullptr));
	OPTIX_CHECK(optixInit());

	ctx_options = {
#ifdef _DEBUG
		.logCallbackFunction = log_cb,
		.logCallbackLevel = 4,
		.validationMode = OPTIX_DEVICE_CONTEXT_VALIDATION_MODE_ALL,
#endif
	};

	OPTIX_CHECK(optixDeviceContextCreate(0, &ctx_options, &ctx));
	OPTIX_CHECK(optixDeviceContextSetCacheEnabled(ctx, 0));

	CU_CHECK(cudaStreamCreate(&stream));

	if (cgltf_parse_file(&gltf_options, gltf_path, &gltf_data) != cgltf_result_success ||
		cgltf_validate(gltf_data) != cgltf_result_success ||
		cgltf_load_buffers(&gltf_options, gltf_data, gltf_path) != cgltf_result_success)
	{
		printf("Error parsing %s\n", gltf_path);
		goto shutdown;
	}

	s = scene_create_from_gltf(gltf_data, ctx, stream);
	RESULT_CHECK(s.result);

	pipeline_compile_options = {
		.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_LEVEL_INSTANCING,
		.numPayloadValues = 0,
		.numAttributeValues = 2,
		.pipelineLaunchParamsVariableName = "lp",
		.usesPrimitiveTypeFlags = (unsigned int)OPTIX_PRIMITIVE_TYPE_FLAGS_TRIANGLE,
	};

	module_compile_options = {
#ifdef _DEBUG
		.optLevel = OPTIX_COMPILE_OPTIMIZATION_LEVEL_0,
		.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_FULL,
#else
		.optLevel = OPTIX_COMPILE_OPTIMIZATION_LEVEL_3,
		.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_NONE,
#endif
	};

	GetModuleFileNameA(GetModuleHandleA(nullptr), curr_dir, MAX_PATH);
	PathRemoveFileSpecA(curr_dir);
	strcat(curr_dir, "/optix/pbr.cu.optixir");

	h_file = (HANDLE)OpenFile(curr_dir, &open_file, OF_READ);

	if (!GetFileSizeEx(h_file, &file_size))
	{
		printf("GetFileSizeEx failed for %s with %d\n", curr_dir, GetLastError());
		goto shutdown;
	}

	module_data = malloc(file_size.QuadPart);

	if (!ReadFile(h_file, module_data, (DWORD)file_size.QuadPart, nullptr, nullptr))
	{
		printf("Could not read module file: %s - %d\n", curr_dir, GetLastError());
		goto shutdown;
	}

	OPTIX_CHECK(optixModuleCreate(ctx, &module_compile_options, &pipeline_compile_options, (char*)module_data, file_size.QuadPart, nullptr, nullptr, &module));

	pipeline_link_options = {
		.maxTraceDepth = 31,
	};

	ray_gen_program_group_desc = {
		.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN,
		.raygen = {
			.module = module,
			.entryFunctionName = "__raygen__rg",
		},
	};

	miss_program_group_desc = {
	  .kind = OPTIX_PROGRAM_GROUP_KIND_MISS,
	  .miss = {
		  .module = module,
		  .entryFunctionName = "__miss__ms",
	  },
	};

	closest_hit_program_group_desc = {
		.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP,
		.hitgroup = {
			.moduleCH = module,
			.entryFunctionNameCH = "__closesthit__ch",
		},
	};

	OPTIX_CHECK(optixProgramGroupCreate(ctx, &ray_gen_program_group_desc, 1, &module_program_group_options, nullptr, nullptr, &ray_gen_program_group));
	OPTIX_CHECK(optixProgramGroupCreate(ctx, &closest_hit_program_group_desc, 1, &module_program_group_options, nullptr, nullptr, &closest_hit_program_group));
	OPTIX_CHECK(optixProgramGroupCreate(ctx, &miss_program_group_desc, 1, &module_program_group_options, nullptr, nullptr, &miss_program_group));

	utils_find_pixel_vecs(s.camera, s.camera.fov, (float)render_width, (float)render_height, pixel_00_loc, pixel_delta_u, pixel_delta_v);

	CU_CHECK(cudaMalloc((void**)&d_rand_states, render_width * render_height * sizeof(curandState)));

	rg_record = {
		.data = {
			.pixel_00_loc = float3(pixel_00_loc[0], pixel_00_loc[1], pixel_00_loc[2]),
			.pixel_delta_u = float3(pixel_delta_u[0], pixel_delta_u[1], pixel_delta_u[2]),
			.pixel_delta_v = float3(pixel_delta_v[0], pixel_delta_v[1], pixel_delta_v[2]),
			.org = float3(s.camera.pos[0], s.camera.pos[1], s.camera.pos[2]),
			.num_samples = num_samples,
			.states = (curandState*)d_rand_states,
		},
	};

	OPTIX_CHECK(optixSbtRecordPackHeader(ray_gen_program_group, (void*)rg_record.header));
	OPTIX_CHECK(optixSbtRecordPackHeader(closest_hit_program_group, closest_hit_record_header));
	OPTIX_CHECK(optixSbtRecordPackHeader(miss_program_group, miss_record_header));

	CU_CHECK(cudaMalloc((void**)&d_ray_gen_record, sizeof(ray_gen_record)));
	CU_CHECK(cudaMemcpy((void*)d_ray_gen_record, (void*)&rg_record, sizeof(ray_gen_record), cudaMemcpyHostToDevice));

	CU_CHECK(cudaMalloc((void**)&d_closest_hit_record, sizeof(ray_gen_record)));
	CU_CHECK(cudaMemcpy((void*)d_closest_hit_record, closest_hit_record_header, OPTIX_SBT_RECORD_HEADER_SIZE, cudaMemcpyHostToDevice));

	CU_CHECK(cudaMalloc((void**)&d_miss_record, sizeof(ray_gen_record)));
	CU_CHECK(cudaMemcpy((void*)d_miss_record, miss_record_header, OPTIX_SBT_RECORD_HEADER_SIZE, cudaMemcpyHostToDevice));

	sbt = {
		.raygenRecord = d_ray_gen_record,
		.missRecordBase = d_miss_record,
		.missRecordStrideInBytes = sizeof(miss_record),
		.missRecordCount = 1,
		.hitgroupRecordBase = d_closest_hit_record,
		.hitgroupRecordStrideInBytes = sizeof(closest_hit_record),
		.hitgroupRecordCount = 1,
	};

	d_exr_passes_staging = reinterpret_cast<exr_pass*>(malloc(sizeof(exr_pass) * passes_count));

	for (size_t p = 0; p < passes_count; ++p)
	{
		d_exr_passes_staging[p].layer = passes[p].layer;
		CU_CHECK(cudaMalloc((void**)(&d_exr_passes_staging[p].d_pixels), render_width * render_height * 4 * sizeof(float)));
	}

	CU_CHECK(cudaMalloc((void**)&d_exr_passes, sizeof(exr_pass) * passes_count));
	CU_CHECK(cudaMemcpy((void*)d_exr_passes, d_exr_passes_staging, sizeof(exr_pass) * passes_count, cudaMemcpyHostToDevice));

	lp = {
		.textures = s.d_textures,
		.materials = s.d_materials,
		.passes = reinterpret_cast<exr_pass*>(d_exr_passes),
		.passes_count = passes_count,
		.render_width = render_width,
		.render_height = render_height,
		.handle = s.ias_hnd,
	};

	CU_CHECK(cudaMalloc((void**)&d_launch_params, sizeof(launch_params)));
	CU_CHECK(cudaMemcpy((void*)d_launch_params, &lp, sizeof(launch_params), cudaMemcpyHostToDevice));
	pipeline_program_groups = reinterpret_cast<OptixProgramGroup*>(malloc(sizeof(OptixProgramGroup) * 3));

	pipeline_program_groups[0] = ray_gen_program_group;
	pipeline_program_groups[1] = miss_program_group;
	pipeline_program_groups[2] = closest_hit_program_group;

	OPTIX_CHECK(optixPipelineCreate(ctx, &pipeline_compile_options, &pipeline_link_options, pipeline_program_groups, 3, nullptr, nullptr, &pipeline));

	OPTIX_CHECK(optixLaunch(pipeline, stream, d_launch_params, sizeof(launch_params), &sbt, (unsigned int)render_width, (unsigned int)render_height, 1));

	CU_CHECK(cudaGetLastError());
	CU_CHECK(cudaStreamSynchronize(stream));

	for (size_t p = 0; p < passes_count; ++p)
	{
		CU_CHECK(cudaMemcpy(passes[p].pixels, (void*)((exr_pass*)d_exr_passes_staging)[p].d_pixels, render_width * render_height * 4 * sizeof(float), cudaMemcpyDeviceToHost));
	}

shutdown:
	if (module_data != nullptr)
	{
		free(module_data);
	}

	scene_destroy(s);

	if (pipeline_program_groups != nullptr)
	{
		free(pipeline_program_groups);
	}

	CU_CHECK(cudaFree((void*)d_rand_states));
	CU_CHECK(cudaFree((void*)d_miss_record));
	CU_CHECK(cudaFree((void*)d_ray_gen_record));
	OPTIX_CHECK(optixProgramGroupDestroy(ray_gen_program_group));
	OPTIX_CHECK(optixProgramGroupDestroy(miss_program_group));
	OPTIX_CHECK(optixModuleDestroy(module));
	OPTIX_CHECK(optixPipelineDestroy(pipeline));
	CU_CHECK(cudaStreamDestroy(stream));
	for (size_t p = 0; p < passes_count; ++p)
	{
		CU_CHECK(cudaFree((void*)((exr_pass*)d_exr_passes_staging)[p].d_pixels));
	}
	free(d_exr_passes_staging);

	CU_CHECK(cudaFree((void*)d_exr_passes));
	CU_CHECK(cudaFree((void*)d_launch_params));
	OPTIX_CHECK(optixDeviceContextDestroy(ctx));

	cgltf_free(gltf_data);

	return;
}