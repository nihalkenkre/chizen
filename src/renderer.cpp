#include "renderer.h"
#include "utils.h"
#include "optix/common.cu.h"

#include <Shlwapi.h>

static void log_cb(unsigned int level, const char* tag, const char* message, void* cbdata)
{
	printf("%d - %s: %s\n", level, tag, message);
}

void renderer_render(const size_t render_width, const size_t render_height, const uint8_t num_samples, const char* gltf_path, exr_pass* passes, const size_t passes_count)
{
	CU_CHECK("init cuda", cudaFree(nullptr));
	OPTIX_CHECK("optix init", optixInit());

	OptixDeviceContext ctx = nullptr;
	const OptixDeviceContextOptions ctx_options = {
#ifdef _DEBUG
		.logCallbackFunction = log_cb,
		.logCallbackLevel = 4,
		.validationMode = OPTIX_DEVICE_CONTEXT_VALIDATION_MODE_ALL,
#endif
	};

	OPTIX_CHECK("optix device context create", optixDeviceContextCreate(0, &ctx_options, &ctx));

	cudaStream_t stream = nullptr;
	CU_CHECK("create stream", cudaStreamCreate(&stream));

	const scene s = scene_create(gltf_path, ctx, stream);

	const OptixPipelineCompileOptions pipeline_compile_options = {
		.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_ANY,
		.numPayloadValues = 8,
		.numAttributeValues = 2,
		.pipelineLaunchParamsVariableName = "lp",
		.usesPrimitiveTypeFlags = (unsigned int)OPTIX_PRIMITIVE_TYPE_FLAGS_TRIANGLE,
	};

	const OptixModuleCompileOptions module_compile_options = {
#ifdef _DEBUG
		.optLevel = OPTIX_COMPILE_OPTIMIZATION_LEVEL_0,
		.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_FULL,
#else
		.optLevel = OPTIX_COMPILE_OPTIMIZATION_LEVEL_3,
		.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_NONE,
#endif
	};

	char curr_dir[MAX_PATH];
	GetModuleFileNameA(GetModuleHandleA(nullptr), curr_dir, MAX_PATH);
	PathRemoveFileSpecA(curr_dir);
	strcat(curr_dir, "/optix/pbr.cu.optixir");

	OFSTRUCT open_file = { 0 };
	HANDLE h_file = (HANDLE)OpenFile(curr_dir, &open_file, OF_READ);

	LARGE_INTEGER file_size = { 0 };
	if (!GetFileSizeEx(h_file, &file_size))
	{
		printf("GetFileSizeEx failed for %s with %d\n", curr_dir, GetLastError());
		return;
	}

	void* module_data = malloc(file_size.QuadPart);

	if (!ReadFile(h_file, module_data, (DWORD)file_size.QuadPart, nullptr, nullptr))
	{
		printf("Could not read module file: %s - %d\n", curr_dir, GetLastError());
		if (module_data != nullptr)
		{
			free(module_data);
		}
		return;
	}

	OptixModule module = nullptr;
	OPTIX_CHECK("create module", optixModuleCreate(ctx, &module_compile_options, &pipeline_compile_options, (char*)module_data, file_size.QuadPart, nullptr, nullptr, &module));

	const OptixPipelineLinkOptions pipeline_link_options = {
		.maxTraceDepth = 31,
	};

	const OptixProgramGroupDesc ray_gen_program_group_desc = {
		.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN,
		.raygen = {
			.module = module,
			.entryFunctionName = "__raygen__rg",
		},
	};

	const OptixProgramGroupDesc miss_program_group_desc = {
		.kind = OPTIX_PROGRAM_GROUP_KIND_MISS,
		.miss = {
			.module = module,
			.entryFunctionName = "__miss__ms",
		},
	};

	const OptixProgramGroupDesc closest_hit_program_group_desc = {
		.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP,
		.hitgroup = {
			.moduleCH = module,
			.entryFunctionNameCH = "__closesthit__ch",
		},
	};

	free(module_data);

	const OptixProgramGroupOptions module_program_group_options = { 0 };

	OptixProgramGroup ray_gen_program_group = nullptr;
	OPTIX_CHECK("create ray_gen program group", optixProgramGroupCreate(ctx, &ray_gen_program_group_desc, 1, &module_program_group_options, nullptr, nullptr, &ray_gen_program_group));

	OptixProgramGroup closest_hit_program_group = nullptr;
	OPTIX_CHECK("create closesthit program group", optixProgramGroupCreate(ctx, &closest_hit_program_group_desc, 1, &module_program_group_options, nullptr, nullptr, &closest_hit_program_group));

	OptixProgramGroup miss_program_group = nullptr;
	OPTIX_CHECK("create miss program group", optixProgramGroupCreate(ctx, &miss_program_group_desc, 1, &module_program_group_options, nullptr, nullptr, &miss_program_group));

	vec3 pixel_00_loc = { 0 }, pixel_delta_u = { 0 }, pixel_delta_v = { 0 };
	utils_find_pixel_vecs(s.camera, s.camera.fov, (float)render_width, (float)render_height, pixel_00_loc, pixel_delta_u, pixel_delta_v);

	CUdeviceptr d_rand_states = 0;
	CU_CHECK("alloc rand states", cudaMalloc((void**)&d_rand_states, render_width * render_height * sizeof(curandState)));

	const ray_gen_record rg_record = {
		.data = {
			.pixel_00_loc = float3(pixel_00_loc[0], pixel_00_loc[1], pixel_00_loc[2]),
			.pixel_delta_u = float3(pixel_delta_u[0], pixel_delta_u[1], pixel_delta_u[2]),
			.pixel_delta_v = float3(pixel_delta_v[0], pixel_delta_v[1], pixel_delta_v[2]),
			.org = float3(s.camera.pos[0], s.camera.pos[1], s.camera.pos[2]),
			.num_samples = num_samples,
			.states = (curandState*)d_rand_states,
		}
	};

	OPTIX_CHECK("pack raygen sbt header", optixSbtRecordPackHeader(ray_gen_program_group, (void*)rg_record.header));

	char closest_hit_record_header[OPTIX_SBT_RECORD_HEADER_SIZE];
	OPTIX_CHECK("pack raygen sbt header", optixSbtRecordPackHeader(closest_hit_program_group, closest_hit_record_header));

	char miss_record_header[OPTIX_SBT_RECORD_HEADER_SIZE];
	OPTIX_CHECK("pack raygen sbt header", optixSbtRecordPackHeader(miss_program_group, miss_record_header));

	CUdeviceptr d_ray_gen_record = 0;
	CU_CHECK("alloc raygen record", cudaMalloc((void**)&d_ray_gen_record, sizeof(ray_gen_record)));
	CU_CHECK("copy raygen sbt record", cudaMemcpy((void*)d_ray_gen_record, (void*)&rg_record, sizeof(ray_gen_record), cudaMemcpyHostToDevice));

	CUdeviceptr d_closest_hit_record = 0;
	CU_CHECK("alloc closest_hit record", cudaMalloc((void**)&d_closest_hit_record, sizeof(ray_gen_record)));
	CU_CHECK("copy closest_hit sbt header", cudaMemcpy((void*)d_closest_hit_record, closest_hit_record_header, OPTIX_SBT_RECORD_HEADER_SIZE, cudaMemcpyHostToDevice));

	CUdeviceptr d_miss_record = 0;
	CU_CHECK("alloc miss record", cudaMalloc((void**)&d_miss_record, sizeof(ray_gen_record)));
	CU_CHECK("copy miss sbt header", cudaMemcpy((void*)d_miss_record, miss_record_header, OPTIX_SBT_RECORD_HEADER_SIZE, cudaMemcpyHostToDevice));

	const OptixShaderBindingTable sbt = {
		.raygenRecord = d_ray_gen_record,
		.missRecordBase = d_miss_record,
		.missRecordStrideInBytes = sizeof(miss_record),
		.missRecordCount = 1,
		.hitgroupRecordBase = d_closest_hit_record,
		.hitgroupRecordStrideInBytes = sizeof(closest_hit_record),
		.hitgroupRecordCount = 1,
	};

	d_exr_pass* d_exr_passes_tmp = reinterpret_cast<d_exr_pass*>(malloc(sizeof(d_exr_pass) * passes_count));

	for (size_t p = 0; p < passes_count; ++p)
	{
		d_exr_passes_tmp[p].layer = passes[p].layer;
		CU_CHECK("alloc d_exr_pass_tmp_pixels", cudaMalloc((void**)(&d_exr_passes_tmp[p].pixels), render_width * render_height * 4 * sizeof(float)));
	}

	CUdeviceptr d_exr_passes = 0;
	CU_CHECK("alloc d_exr_passes", cudaMalloc((void**)&d_exr_passes, sizeof(d_exr_pass) * passes_count));
	CU_CHECK("copy d_exr_passes", cudaMemcpy((void*)d_exr_passes, d_exr_passes_tmp, sizeof(exr_pass) * passes_count, cudaMemcpyHostToDevice));

	const launch_params lp = {
		.passes = (exr_pass*)d_exr_passes,
		.passes_count = passes_count,
		.render_width = render_width,
		.render_height = render_height,
		.handle = s.ias_hnd,
	};

	CUdeviceptr d_launch_params = 0;
	CU_CHECK("alloc d_launch_params", cudaMalloc((void**)&d_launch_params, sizeof(launch_params)));
	CU_CHECK("copy lp to device", cudaMemcpy((void*)d_launch_params, &lp, sizeof(launch_params), cudaMemcpyHostToDevice));
	const OptixProgramGroup pipeline_program_groups[] = {
		ray_gen_program_group,
		miss_program_group,
		closest_hit_program_group,
	};

	OptixPipeline pipeline = nullptr;
	OPTIX_CHECK("create pipeline", optixPipelineCreate(ctx, &pipeline_compile_options, &pipeline_link_options, pipeline_program_groups, _countof(pipeline_program_groups), nullptr, nullptr, &pipeline));

	OPTIX_CHECK("launch optix", optixLaunch(pipeline, stream, d_launch_params, sizeof(launch_params), &sbt, (unsigned int)render_width, (unsigned int)render_height, 1));

	CU_CHECK("optix kernel", cudaGetLastError());
	CU_CHECK("stream sync", cudaStreamSynchronize(stream));

	for (size_t p = 0; p < passes_count; ++p)
	{
		CU_CHECK("copy pixels to host", cudaMemcpy(passes[p].pixels, (void*)((d_exr_pass*)d_exr_passes_tmp)[p].pixels, render_width * render_height * 4 * sizeof(float), cudaMemcpyDeviceToHost));
	}

	scene_destroy(s);

	CU_CHECK("dealloc rand states", cudaFree((void*)d_rand_states));
	CU_CHECK("dealloc miss record ptr", cudaFree((void*)d_miss_record));
	CU_CHECK("dealloc ray gen record", cudaFree((void*)d_ray_gen_record));
	OPTIX_CHECK("destroy program group", optixProgramGroupDestroy(ray_gen_program_group));
	OPTIX_CHECK("destroy program group", optixProgramGroupDestroy(miss_program_group));
	OPTIX_CHECK("destroy module", optixModuleDestroy(module));
	OPTIX_CHECK("pipeline destroy", optixPipelineDestroy(pipeline));
	CU_CHECK("destroy stream", cudaStreamDestroy(stream));
	for (size_t p = 0; p < passes_count; ++p)
	{
		CU_CHECK("dealloc d_pixels", cudaFree((void*) ((d_exr_pass*)d_exr_passes_tmp)[p].pixels));
	}
	free(d_exr_passes_tmp);

	CU_CHECK("dealloc d_passes_pixels", cudaFree((void*)d_exr_passes));
	CU_CHECK("dealloc d_launch_params", cudaFree((void*)d_launch_params));
	OPTIX_CHECK("optix device context destroy", optixDeviceContextDestroy(ctx));
}
