#include "renderer.h"
#include "utils.h"
#include "common.h"
#include "optix/helpers.cuh"

#include <curand_kernel.h>
#include <cuda_runtime.h>

#include <optix.h>
#include <optix_stubs.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <optix_stack_size.h>
#include <optix_function_table_definition.h>

#include <Shlwapi.h>
#include <ctime>

#define NUM_SAMPLES 2048

static void log_cb(unsigned int level, const char* tag, const char* message, void* cbdata)
{
	printf("%d - %s: %s\n", level, tag, message);
}

CHIZEN_RESULT renderer_render_gltf(const uint32_t render_width, const uint32_t render_height, const char* gltf_path, exr_pass* passes, const uint32_t passes_count)
{
	CHIZEN_RESULT chi_result = CHIZEN_RESULT_SUCCESS;
	cudaError_t cuda_error = cudaSuccess;
	OptixResult optix_result = OPTIX_SUCCESS;

	cgltf_options gltf_options = {};
	cgltf_data* gltf_data = nullptr;
	OptixDeviceContext ctx = nullptr;
	OptixDeviceContextOptions ctx_options = {};
	scene s = { };
	OptixPipelineCompileOptions pipeline_compile_options = {};
	OptixModuleCompileOptions module_compile_options = {};
	char curr_dir[MAX_PATH];
	OFSTRUCT open_file = {};
	HANDLE h_file = nullptr;
	LARGE_INTEGER file_size = {};
	void* module_data = nullptr;
	OptixModule module = nullptr;
	OptixPipelineLinkOptions pipeline_link_options = {};
	OptixProgramGroupDesc rg_pg_desc = {};
	OptixProgramGroupDesc ms_od_pg_desc = {};
	OptixProgramGroupDesc ms_ld_pg_desc = {};
	OptixProgramGroupDesc ch_od_pg_desc = {};
	OptixProgramGroupDesc ch_ld_pg_desc = {};
	OptixProgramGroupOptions pg_options = { 0 };
	OptixProgramGroup rg_pg = nullptr;
	OptixProgramGroup ch_od_pg = nullptr;
	OptixProgramGroup ch_ld_pg = nullptr;
	OptixProgramGroup ms_od_pg = nullptr;
	OptixProgramGroup ms_ld_pg = nullptr;
	vec3 pixel_00_loc = { 0 }, pixel_delta_u = { 0 }, pixel_delta_v = { 0 };
	ray_gen_record rg_record = {};
	ch_record* ch_records = nullptr;
	ms_record ms_od_record = {};
	ms_record ms_ld_record = {};
	CUdeviceptr d_rand_states = 0;
	CUdeviceptr d_rg_record_base = 0;
	CUdeviceptr d_ch_record_base = 0;
	CUdeviceptr d_ms_record_base = 0;
	OptixShaderBindingTable sbt = {};
	exr_pass* d_exr_passes_staging = nullptr;
	CUdeviceptr d_exr_passes = 0;
	launch_params lp = {};
	CUdeviceptr d_launch_params = 0;
	OptixPipeline pipeline = nullptr;
	OptixProgramGroup pipeline_pgs[5] = {};
	size_t ch_records_size = 0;

	CU_CHECK("init", cudaFree(nullptr), chi_result);
	OPTIX_CHECK("init", optixInit(), chi_result);

	ctx_options = {
#ifdef _DEBUG
		.logCallbackFunction = log_cb,
		.logCallbackLevel = 4,
		.validationMode = OPTIX_DEVICE_CONTEXT_VALIDATION_MODE_ALL,
#endif
	};

	OPTIX_CHECK("ctx create", optixDeviceContextCreate(0, &ctx_options, &ctx), chi_result);

	if (cgltf_parse_file(&gltf_options, gltf_path, &gltf_data) != cgltf_result_success ||
		cgltf_validate(gltf_data) != cgltf_result_success ||
		cgltf_load_buffers(&gltf_options, gltf_data, gltf_path) != cgltf_result_success)
	{
		printf("Error parsing %s\n", gltf_path);
		goto cpu_error;
	}

	pipeline_compile_options = {
		.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_LEVEL_INSTANCING,
		.numPayloadValues = 3,
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
		chi_result = CHIZEN_RESULT_FILE_ERROR;
		goto cpu_error;
	}

	module_data = calloc(1, file_size.QuadPart);
	if (module_data == NULL)
	{
		printf("calloc failed for module_data\n");
		chi_result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
		goto cpu_error;
	}

	if (!ReadFile(h_file, module_data, (DWORD)file_size.QuadPart, nullptr, nullptr))
	{
		printf("Could not read module file: %s - %d\n", curr_dir, GetLastError());
		chi_result = CHIZEN_RESULT_FILE_ERROR;
		goto cpu_error;
	}

	OPTIX_CHECK("module create", optixModuleCreate(ctx, &module_compile_options, &pipeline_compile_options, (char*)module_data, file_size.QuadPart, nullptr, nullptr, &module), chi_result);

	ch_od_pg_desc = {
		.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP,
		.hitgroup = {
			.moduleCH = module,
			.entryFunctionNameCH = "__closesthit__od",
		},
	};

	ch_ld_pg_desc = {
		.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP,
		.hitgroup = {
			.moduleCH = module,
			.entryFunctionNameCH = "__closesthit__ld",
		},
	};

	OPTIX_CHECK("ch od program group create", optixProgramGroupCreate(ctx, &ch_od_pg_desc, 1, &pg_options, nullptr, nullptr, &ch_od_pg), chi_result);
	OPTIX_CHECK("ch ld program group create", optixProgramGroupCreate(ctx, &ch_ld_pg_desc, 1, &pg_options, nullptr, nullptr, &ch_ld_pg), chi_result);
	s = scene_create_from_gltf(gltf_data, ch_od_pg, ch_ld_pg, ctx, 0);
	CHIZEN_RESULT_CHECK("scene create", s.result, chi_result);

	pipeline_link_options = {
		.maxTraceDepth = 1,
	};

	ch_records_size = sizeof(ch_record) * s.ch_infos.count;
	CU_CHECK("alloc d_ch_records", cudaMalloc((void**)&d_ch_record_base, ch_records_size), chi_result);
	CU_CHECK("copy ch_records to device", cudaMemcpy((void*)d_ch_record_base, s.ch_infos.ch_records, ch_records_size, cudaMemcpyHostToDevice), chi_result);

	utils_find_pixel_vecs(s.camera, s.camera.fov, (float)render_width, (float)render_height, pixel_00_loc, pixel_delta_u, pixel_delta_v);

	rg_record = {
		.data = {
			.pixel_00_loc = float3(pixel_00_loc[0], pixel_00_loc[1], pixel_00_loc[2]),
			.pixel_delta_u = float3(pixel_delta_u[0], pixel_delta_u[1], pixel_delta_u[2]),
			.pixel_delta_v = float3(pixel_delta_v[0], pixel_delta_v[1], pixel_delta_v[2]),
			.org = float3(s.camera.pos[0], s.camera.pos[1], s.camera.pos[2]),
		},
	};

	rg_pg_desc = {
		.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN,
		.raygen = {
			.module = module,
			.entryFunctionName = "__raygen__rg",
		},
	};

	ms_od_pg_desc = {
	  .kind = OPTIX_PROGRAM_GROUP_KIND_MISS,
	  .miss = {
		  .module = module,
		  .entryFunctionName = "__miss__od",
	  },
	};

	ms_ld_pg_desc = {
	  .kind = OPTIX_PROGRAM_GROUP_KIND_MISS,
	  .miss = {
		  .module = module,
		  .entryFunctionName = "__miss__ld",
	  },
	};

	OPTIX_CHECK("rg program group create", optixProgramGroupCreate(ctx, &rg_pg_desc, 1, &pg_options, nullptr, nullptr, &rg_pg), chi_result);
	OPTIX_CHECK("ms rg program group create", optixProgramGroupCreate(ctx, &ms_od_pg_desc, 1, &pg_options, nullptr, nullptr, &ms_od_pg), chi_result);
	OPTIX_CHECK("ms rg program group create", optixProgramGroupCreate(ctx, &ms_ld_pg_desc, 1, &pg_options, nullptr, nullptr, &ms_ld_pg), chi_result);

	OPTIX_CHECK("rg sbt pack header", optixSbtRecordPackHeader(rg_pg, (void*)rg_record.header), chi_result);
	OPTIX_CHECK("ms sbt pack header", optixSbtRecordPackHeader(ms_od_pg, (void*)ms_od_record.header), chi_result);
	OPTIX_CHECK("ms sbt pack header", optixSbtRecordPackHeader(ms_ld_pg, (void*)ms_ld_record.header), chi_result);

	CU_CHECK("alloc rg record", cudaMalloc((void**)&d_rg_record_base, sizeof(ray_gen_record)), chi_result);
	CU_CHECK("copy rg to device", cudaMemcpy((void*)d_rg_record_base, (void*)&rg_record, sizeof(ray_gen_record), cudaMemcpyHostToDevice), chi_result);

	CU_CHECK("alloc ms record", cudaMalloc((void**)&d_ms_record_base, sizeof(ms_record) * RAY_TYPE_MAX), chi_result);
	CU_CHECK("copy ms rg to device", cudaMemcpy((void*)d_ms_record_base, (void*)&ms_od_record, sizeof(ms_record), cudaMemcpyHostToDevice), chi_result);
	CU_CHECK("copy ms b to device", cudaMemcpy((void*)(d_ms_record_base + sizeof(ms_record)), (void*)&ms_ld_record, sizeof(ms_record), cudaMemcpyHostToDevice), chi_result);

	sbt = {
		.raygenRecord = d_rg_record_base,
		.missRecordBase = d_ms_record_base,
		.missRecordStrideInBytes = sizeof(ms_record),
		.missRecordCount = RAY_TYPE_MAX,
		.hitgroupRecordBase = d_ch_record_base,
		.hitgroupRecordStrideInBytes = sizeof(ch_record),
		.hitgroupRecordCount = (unsigned int)s.ch_infos.count,
	};

	d_exr_passes_staging = (exr_pass*)calloc(passes_count, sizeof(exr_pass));
	if (d_exr_passes_staging == nullptr)
	{
		printf("calloc failed for d_exr_passes_staging\n");
		chi_result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
		goto cpu_error;
	}

	for (size_t p = 0; p < passes_count; ++p)
	{
		d_exr_passes_staging[p].layer = passes[p].layer;
		CU_CHECK("alloc d_pixels for exr staging", cudaMalloc((void**)(&d_exr_passes_staging[p].d_pixels), render_width * render_height * passes[p].layer.num_channels * sizeof(float)), chi_result);
	}

	CU_CHECK("alloc d_exr_passes", cudaMalloc((void**)&d_exr_passes, sizeof(exr_pass) * passes_count), chi_result);
	CU_CHECK("copy d_exr staging to final", cudaMemcpy((void*)d_exr_passes, d_exr_passes_staging, sizeof(exr_pass) * passes_count, cudaMemcpyHostToDevice), chi_result);

	srand((unsigned int)time(NULL));
	CU_CHECK("alloc rand states", cudaMalloc((void**)&d_rand_states, render_width * render_height * sizeof(curandState)), chi_result);
	lp = {
		.textures = s.d_textures,
		.materials = s.d_materials,
		.passes = (exr_pass*)d_exr_passes,
		.passes_count = passes_count,
		.lights = s.d_lights,
		.lights_count = s.d_lights_count,
		.render_width = render_width,
		.render_height = render_height,
		.max_bounces = pipeline_link_options.maxTraceDepth,
		.states = (void*)d_rand_states,
		.handle = s.ias_hnd,
	};

	CU_CHECK("alloc lp", cudaMalloc((void**)&d_launch_params, sizeof(launch_params)), chi_result);
	CU_CHECK("copy lp to device", cudaMemcpy((void*)d_launch_params, &lp, sizeof(launch_params), cudaMemcpyHostToDevice), chi_result);

	pipeline_pgs[0] = rg_pg;
	pipeline_pgs[1] = ms_od_pg;
	pipeline_pgs[2] = ms_ld_pg;
	pipeline_pgs[3] = ch_od_pg;
	pipeline_pgs[4] = ch_ld_pg;

	OPTIX_CHECK("create pipeline", optixPipelineCreate(ctx, &pipeline_compile_options, &pipeline_link_options, pipeline_pgs, _countof(pipeline_pgs), nullptr, nullptr, &pipeline), chi_result);

	printf("generating random states...\n");
	generate_random_states(lp.states, render_width, render_height, 0);

	for (uint32_t s = 0; s < NUM_SAMPLES; ++s)
	{
		printf("\rProcess sample %u...", s+1);
		OPTIX_CHECK("launch", optixLaunch(pipeline, 0, d_launch_params, sizeof(launch_params), &sbt, (unsigned int)render_width, (unsigned int)render_height, 1), chi_result);
	}
	printf("\n");
	
	printf("averaging pixels...\n");
	avg_ld_pixels((void*)d_exr_passes, passes_count, render_width, render_height, NUM_SAMPLES, 0);

	CU_CHECK("get last error", cudaGetLastError(), chi_result);
	CU_CHECK("stream sync", cudaStreamSynchronize(0), chi_result);

	printf("copying pixels to host...\n");
	for (size_t p = 0; p < passes_count; ++p)
	{
		CU_CHECK("copy pass pixels to host", cudaMemcpy(passes[p].pixels, (void*)((exr_pass*)d_exr_passes_staging)[p].d_pixels, render_width * render_height * passes[p].layer.num_channels * sizeof(float), cudaMemcpyDeviceToHost), chi_result);
	}

cpu_error:
	CU_CHECK("free rand states", cudaFree((void*)d_rand_states), chi_result);
	CU_CHECK("free rg record base", cudaFree((void*)d_rg_record_base), chi_result);
	CU_CHECK("free ms record base", cudaFree((void*)d_ms_record_base), chi_result);
	CU_CHECK("free ch record base", cudaFree((void*)d_ch_record_base), chi_result);
	OPTIX_CHECK("rg pg destroy", optixProgramGroupDestroy(rg_pg), chi_result);
	OPTIX_CHECK("ms pg destroy", optixProgramGroupDestroy(ms_od_pg), chi_result);
	OPTIX_CHECK("ch pg destroy", optixProgramGroupDestroy(ch_od_pg), chi_result);
	OPTIX_CHECK("module destroy", optixModuleDestroy(module), chi_result);
	OPTIX_CHECK("pipeline destroy", optixPipelineDestroy(pipeline), chi_result);

	if (d_exr_passes_staging != NULL)
	{
		for (size_t p = 0; p < passes_count; ++p)
		{
			CU_CHECK("free exr d_pixels", cudaFree((void*)d_exr_passes_staging[p].d_pixels), chi_result);
		}
	}

	CU_CHECK("free d_exr_passes", cudaFree((void*)d_exr_passes), chi_result);
	CU_CHECK("free launch params", cudaFree((void*)d_launch_params), chi_result);
	OPTIX_CHECK("destroy context", optixDeviceContextDestroy(ctx), chi_result);

gpu_error:
	free(module_data);
	free(d_exr_passes_staging);
	scene_destroy(s);
	cgltf_free(gltf_data);

	return chi_result;
}