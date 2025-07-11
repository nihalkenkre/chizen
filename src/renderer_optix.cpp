#include "renderer.h"
#include "../optix/common.h"
#include "utils.h"

#include <Shlwapi.h>

static inline void CU_CHECK(const char* action, const cudaError_t result)
{
	if (result > cudaSuccess)
	{
		printf("CUDA ERR %d: %s\nExiting...\n", result, action);
		exit(result);
	}
}

static inline void OPTIX_CHECK(const char* action, const OptixResult result)
{
	if (result > OPTIX_SUCCESS)
	{
		printf("ERR: %s %s\n", action, optixGetErrorName(result));
		exit(result);
	}
}

static void log_cb(unsigned int level, const char* tag, const char* message, void* cbdata)
{
	printf("%d - %s: %s\n", level, tag, message);
}

void renderer_render_optix(const size_t render_width, const size_t render_height, const uint8_t num_samples, const char* gltf_path, uint8_t* pixels)
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

	const scene_optix s = scene_optix_create(gltf_path, ctx, stream);

	const OptixPipelineCompileOptions pipeline_compile_options = {
		.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_ANY,
		.numPayloadValues = 3,
		.numAttributeValues = 2,
		.pipelineLaunchParamsVariableName = "lp",
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
	strcat(curr_dir, "/optix/sphere.cu.optixir");

	OFSTRUCT open_file = { 0 };
	HANDLE h_file = (HANDLE)OpenFile(curr_dir, &open_file, OF_READ);

	unsigned int module_data_size = (unsigned int)GetFileSize(h_file, nullptr);
	void* module_data = malloc(module_data_size);

	if (!ReadFile(h_file, module_data, module_data_size, nullptr, nullptr))
	{
		printf("Could not read module file: %s - %d\n", curr_dir, GetLastError());
		if (module_data != nullptr)
		{
			free(module_data);
		}
		return;
	}

	OptixModule module = nullptr;
	OPTIX_CHECK("create module", optixModuleCreate(ctx, &module_compile_options, &pipeline_compile_options, (char*)module_data, module_data_size, nullptr, nullptr, &module));

	const OptixPipelineLinkOptions pipeline_link_options = {
		.maxTraceDepth = 16,
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

	const OptixProgramGroupOptions module_program_group_options = { 0 };

	OptixProgramGroup ray_gen_program_group = nullptr;
	OPTIX_CHECK("create program group", optixProgramGroupCreate(ctx, &ray_gen_program_group_desc, 1, &module_program_group_options, nullptr, nullptr, &ray_gen_program_group));

	OptixProgramGroup miss_program_group = nullptr;
	OPTIX_CHECK("create program group", optixProgramGroupCreate(ctx, &miss_program_group_desc, 1, &module_program_group_options, nullptr, nullptr, &miss_program_group));

	vec3 pixel_00_loc = { 0 }, pixel_delta_u = { 0 }, pixel_delta_v = { 0 };
	utils_find_pixel_vecs(s.camera, s.camera.fov, (float)render_width, (float)render_height, pixel_00_loc, pixel_delta_u, pixel_delta_v);

	const ray_gen_record_data rg_record_data = {
		.pixel_00_loc = float3(pixel_00_loc[0], pixel_00_loc[1], pixel_00_loc[2]),
		.pixel_delta_u = float3(pixel_delta_u[0], pixel_delta_u[1], pixel_delta_u[2]),
		.pixel_delta_v = float3(pixel_delta_v[0], pixel_delta_v[1], pixel_delta_v[2]),
		.org = float3(s.camera.pos[0], s.camera.pos[1], s.camera.pos[2]),
	};

	ray_gen_record rg_record = {
		.data = {
			.pixel_00_loc = float3(pixel_00_loc[0], pixel_00_loc[1], pixel_00_loc[2]),
			.pixel_delta_u = float3(pixel_delta_u[0], pixel_delta_u[1], pixel_delta_u[2]),
			.pixel_delta_v = float3(pixel_delta_v[0], pixel_delta_v[1], pixel_delta_v[2]),
			.org = float3(s.camera.pos[0], s.camera.pos[1], s.camera.pos[2]),
		}
	};
	OPTIX_CHECK("pack raygen sbt header", optixSbtRecordPackHeader(ray_gen_program_group, &rg_record.header));

	char miss_record_header[OPTIX_SBT_RECORD_HEADER_SIZE];
	OPTIX_CHECK("pack raygen sbt header", optixSbtRecordPackHeader(miss_program_group, miss_record_header));

	CUdeviceptr d_ray_gen_record = 0;
	CU_CHECK("alloc raygen record", cudaMalloc((void**)&d_ray_gen_record, sizeof(ray_gen_record)));
	CU_CHECK("copy raygen sbt record", cudaMemcpy((void*)d_ray_gen_record, (void*)&rg_record, sizeof(ray_gen_record), cudaMemcpyHostToDevice));

	CUdeviceptr d_miss_record = 0;
	CU_CHECK("alloc miss record", cudaMalloc((void**)&d_miss_record, sizeof(ray_gen_record)));
	CU_CHECK("copy miss sbt header", cudaMemcpy((void*)d_miss_record, miss_record_header, OPTIX_SBT_RECORD_HEADER_SIZE, cudaMemcpyHostToDevice));

	const OptixShaderBindingTable sbt = {
		.raygenRecord = d_ray_gen_record,
		.missRecordBase = d_miss_record,
		.missRecordStrideInBytes = sizeof(miss_record),
		.missRecordCount = 1,
	};

	CUdeviceptr d_pixels = 0;
	CU_CHECK("alloc d_pixels", cudaMalloc((void**)&d_pixels, render_width * render_height * 4));

	const launch_params lp = {
		.pixels = (uint8_t*)d_pixels,
		.render_width = render_width,
		.render_height = render_height,
	};

	CUdeviceptr d_launch_params = 0;
	CU_CHECK("alloc d_launch_params", cudaMalloc((void**)&d_launch_params, sizeof(launch_params)));
	CU_CHECK("copy lp to device", cudaMemcpy((void*)d_launch_params, &lp, sizeof(launch_params), cudaMemcpyHostToDevice));
	const OptixProgramGroup pipeline_program_groups[] = {
		ray_gen_program_group,
		miss_program_group,
	};

	OptixPipeline pipeline = nullptr;
	OPTIX_CHECK("create pipeline", optixPipelineCreate(ctx, &pipeline_compile_options, &pipeline_link_options, pipeline_program_groups, _countof(pipeline_program_groups), nullptr, nullptr, &pipeline));

	OPTIX_CHECK("launch optix", optixLaunch(pipeline, stream, d_launch_params, sizeof(launch_params), &sbt, (unsigned int)render_width, (unsigned int)render_height, 1));

	CU_CHECK("cuda sync", cudaStreamSynchronize(stream));

	CU_CHECK("copy pixels to host", cudaMemcpy(pixels, (void*)d_pixels, render_width * render_height * 4, cudaMemcpyDeviceToHost));

	scene_optix_destroy(s);

	CU_CHECK("dealloc miss record ptr", cudaFree((void*)d_miss_record));
	CU_CHECK("dealloc ray gen record", cudaFree((void*)d_ray_gen_record));
	OPTIX_CHECK("destroy program group", optixProgramGroupDestroy(ray_gen_program_group));
	OPTIX_CHECK("destroy program group", optixProgramGroupDestroy(miss_program_group));
	OPTIX_CHECK("destroy module", optixModuleDestroy(module));
	OPTIX_CHECK("pipeline destroy", optixPipelineDestroy(pipeline));
	CU_CHECK("destroy stream", cudaStreamDestroy(stream));
	CU_CHECK("dealloc d_pixels", cudaFree((void*)d_pixels));
	CU_CHECK("dealloc d_launch_params", cudaFree((void*)d_launch_params));
	OPTIX_CHECK("optix device context destroy", optixDeviceContextDestroy(ctx));
}
