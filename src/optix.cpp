#include "optix.hpp"

#include <Shlwapi.h>

#include <cstdio>
#include <cstdlib>

#include <optix.h>
#include <optix_stubs.h>
#include <cuda_runtime.h>

#include "utils.h"

#define CGLM_FORCE_ZERO_TO_ONE
#include <cglm/include/cglm/cglm.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <optix_stack_size.h>
#include <optix_function_table_definition.h>
#include <optix_micromap.h>

struct Params
{
    uchar4 *image;
    unsigned int image_width;
    unsigned int image_height;
    OptixTraversableHandle handle;
};

struct RaygenData
{
    float3 pixel_delta;
    float3 pixel_00_loc;
    float3 cam_eye;
};

struct MissData
{
    unsigned int r, g, b;
};

struct HitGroupData
{
};

struct rg_sbt_record
{
    __align__(OPTIX_SBT_RECORD_ALIGNMENT) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    RaygenData data;
};

struct ms_sbt_record
{
    __align__(OPTIX_SBT_RECORD_ALIGNMENT) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    MissData data;
};

struct hg_sbt_record
{
    __align__(OPTIX_SBT_RECORD_ALIGNMENT) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    HitGroupData data;
};

static inline void OPTIX_CHECK(const char *action, const OptixResult result)
{
    if (result > 0)
    {
        printf("OPTIX ERR %d: %s\nExiting...\n", result, action);
        exit(result);
    }
}

static inline void CU_CHECK(const char *action, const cudaError result)
{
    if (result > 0)
    {
        printf("CUDA ERR %d: %s\nExiting...\n", result, action);
        exit(result);
    }
}

static inline void CU_CHECK(const char *action, const CUresult result)
{
    if (result > 0)
    {
        printf("CUDA RESULT %d: %s\nExiting...\n", result, action);
        exit(result);
    }
}

static void optx_log_cb(unsigned int level, const char *tag, const char *message, void *cb_data)
{
    printf("Level: %d %s %s\n", level, tag, message);
}

void optix_render(const uint32_t render_width, const uint32_t render_height, uint8_t *pixels)
{
    vec3 cam_eye = {0, 0, 10};
    const float viewport_height = 2.f;
    const float viewport_width = 2.f * (static_cast<float>(render_width) / static_cast<float>(render_height));
    vec3 viewport_u = {viewport_width, 0, 0};
    vec3 viewport_v = {0, -viewport_height, 0};
    vec3 pixel_delta_u = {0};
    glm_vec3_div(viewport_u, vec3{static_cast<float>(render_width), static_cast<float>(render_width), static_cast<float>(render_width)}, pixel_delta_u);

    vec3 pixel_delta_v = {0};
    glm_vec3_div(viewport_v, vec3{static_cast<float>(render_height), static_cast<float>(render_height), static_cast<float>(render_height)}, pixel_delta_v);

    vec3 viewport_uv = {0};
    glm_vec2_add(viewport_u, viewport_v, viewport_uv);
    vec3 viewport_upper_left = {0};
    glm_vec3_sub(cam_eye, vec3{0, 0, 1.f}, viewport_upper_left); // focal length
    glm_vec3_sub(viewport_upper_left, viewport_uv, viewport_upper_left);

    glm_vec3_div(viewport_upper_left, vec3{2, 2, 1}, viewport_upper_left);

    vec3 pixel_delta = {0};
    glm_vec3_add(pixel_delta_u, pixel_delta_v, pixel_delta);

    vec3 pixel_00_loc = {0};
    glm_vec3_mul(pixel_delta, vec3{0.5, 0.5, 1}, pixel_00_loc);
    glm_vec3_add(viewport_upper_left, pixel_00_loc, pixel_00_loc);

    // for (uint32_t y = 0; y < render_height; ++y)
    //{
    //     for (uint32_t x = 0; x < render_width; ++x)
    //     {
    //         vec3 pixel_center = {
    //             pixel_00_loc[0] + (x * pixel_delta_u),
    //             pixel_00_loc[1] + (y * pixel_delta_v),
    //             pixel_00_loc[2],  // focal length
    //         };

    //        vec3 ray_dir = {};
    //        glm_vec3_sub(pixel_center, cam_eye, ray_dir);
    //        glm_vec3_normalize(ray_dir);

    //        ray_dir[1] = (ray_dir[1] + 1) * 0.5f;

    //        pixels[(render_width * y * 4) + (x * 4)] = 0;// (uint8_t)(ray_dir[2] * 255);
    //        pixels[(render_width * y * 4) + (x * 4 + 1)] = (uint8_t)(ray_dir[1] * 255);
    //        pixels[(render_width * y * 4) + (x * 4 + 2)] = 0;// (uint8_t)(ray_dir[0] * 255);
    //        pixels[(render_width * y * 4) + (x * 4 + 3)] = 255;
    //    }
    //}

    printf("============== BEGIN OPTIX LOG ==============\n");
    OptixDeviceContext optix_dev_ctx = 0;
    OptixDeviceContextOptions options = {};
    OptixModule module = nullptr;
    OptixModule sphere_module = nullptr;
    OptixModuleCompileOptions module_compile_options = {};
    OptixBuiltinISOptions builtin_is_options = {};
    OptixPipelineCompileOptions pipeline_compile_options = {};
    char curr_dir[MAX_PATH];
    OFSTRUCT open_file_struct = {};
    OptixProgramGroupDesc rg_prog_group_desc = {};
    OptixProgramGroup rg_prog_group = nullptr;
    OptixProgramGroupDesc ms_prog_group_desc = {};
    OptixProgramGroup ms_prog_group = nullptr;
    OptixProgramGroupDesc hg_prog_group_desc = {};
    OptixProgramGroup hg_prog_group = nullptr;
    OptixProgramGroupOptions prog_group_options = {};
    OptixPipelineLinkOptions pipeline_link_options = {};
    OptixProgramGroup program_groups[3];
    OptixPipeline pipeline = nullptr;
    OptixStackSizes stack_sizes = {};
    OptixShaderBindingTable sbt = {};
    CUdeviceptr rg_record_ptr = 0;
    CUdeviceptr ms_record_ptr = 0;
    CUdeviceptr hg_record_ptr = 0;
    uchar4 *output_buffer = nullptr;
    Params params = {};
    CUdeviceptr d_params = 0;
    CUstream stream = nullptr;
    rg_sbt_record rg_sbt = {};
    ms_sbt_record ms_sbt = {};
    hg_sbt_record hg_sbt = {};

    CU_CHECK("init cuda", cudaFree(nullptr));
    OPTIX_CHECK("init optix", optixInit());

#ifdef DEBUG
    options.logCallbackFunction = optx_log_cb;
    options.logCallbackLevel = 4;
    options.validationMode = OPTIX_DEVICE_CONTEXT_VALIDATION_MODE_ALL;
#endif // DEBUG

    OPTIX_CHECK("create device context", optixDeviceContextCreate(0, &options, &optix_dev_ctx));

    OptixTraversableHandle gas_handle;

    CUdeviceptr d_gas_output_buffer;
    OptixAccelBuildOptions accel_options = {0};
    accel_options.buildFlags = OPTIX_BUILD_FLAG_ALLOW_COMPACTION | OPTIX_BUILD_FLAG_ALLOW_RANDOM_VERTEX_ACCESS;
    accel_options.operation = OPTIX_BUILD_OPERATION_BUILD;

    float3 sphere_vertex = make_float3(0, 0, 0);
    float sphere_radius = 3.f;

    CUdeviceptr d_vertex_buffer;
    CU_CHECK("allocate vertex buffer", cudaMalloc(reinterpret_cast<void **>(&d_vertex_buffer), sizeof(float3)));
    CU_CHECK("copy vertex data", cudaMemcpy(reinterpret_cast<void *>(d_vertex_buffer), &sphere_vertex, sizeof(float3), cudaMemcpyHostToDevice));

    CUdeviceptr d_radius_buffer;
    CU_CHECK("allocate radius buffer", cudaMalloc(reinterpret_cast<void **>(&d_radius_buffer), sizeof(float3)));
    CU_CHECK("copy radius data", cudaMemcpy(reinterpret_cast<void *>(d_radius_buffer), &sphere_radius, sizeof(float3), cudaMemcpyHostToDevice));

    OptixBuildInput sphere_input = {0};
    sphere_input.type = OPTIX_BUILD_INPUT_TYPE_SPHERES;
    sphere_input.sphereArray.vertexBuffers = &d_vertex_buffer;
    sphere_input.sphereArray.numVertices = 1;
    sphere_input.sphereArray.radiusBuffers = &d_radius_buffer;
    uint32_t sphere_input_flags[1] = {OPTIX_GEOMETRY_FLAG_NONE};
    sphere_input.sphereArray.flags = sphere_input_flags;
    sphere_input.sphereArray.numSbtRecords = 1;

    OptixAccelBufferSizes gas_buffer_sizes;
    OPTIX_CHECK("compute accel sizes", optixAccelComputeMemoryUsage(optix_dev_ctx, &accel_options, &sphere_input, 1, &gas_buffer_sizes));

    CUdeviceptr d_gas_tmp_buffer;
    CU_CHECK("alloc tmp gas buff", cudaMalloc(reinterpret_cast<void **>(&d_gas_tmp_buffer), gas_buffer_sizes.tempSizeInBytes));
    CU_CHECK("alloc out gas buff", cudaMalloc(reinterpret_cast<void **>(&d_gas_output_buffer), gas_buffer_sizes.outputSizeInBytes));

    OPTIX_CHECK("accel build", optixAccelBuild(optix_dev_ctx, 0, &accel_options, &sphere_input, 1, d_gas_tmp_buffer, gas_buffer_sizes.tempSizeInBytes, d_gas_output_buffer, gas_buffer_sizes.outputSizeInBytes, &gas_handle, nullptr, 0));
    CU_CHECK("free tmp gas buffer", cudaFree(reinterpret_cast<void *>(d_gas_tmp_buffer)));
    CU_CHECK("free sphere vertex", cudaFree(reinterpret_cast<void *>(d_vertex_buffer)));
    CU_CHECK("free radius vertex", cudaFree(reinterpret_cast<void *>(d_radius_buffer)));

#ifdef DEBUG
    module_compile_options.optLevel = OPTIX_COMPILE_OPTIMIZATION_LEVEL_0;
    module_compile_options.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_FULL;
#endif // DEBUG

    pipeline_compile_options.usesPrimitiveTypeFlags = OPTIX_PRIMITIVE_TYPE_FLAGS_SPHERE;
    pipeline_compile_options.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
    pipeline_compile_options.numPayloadValues = 3;
    pipeline_compile_options.numAttributeValues = 2;
    pipeline_compile_options.exceptionFlags = OPTIX_EXCEPTION_FLAG_STACK_OVERFLOW | OPTIX_EXCEPTION_FLAG_TRACE_DEPTH;
    pipeline_compile_options.pipelineLaunchParamsVariableName = "params";

    GetModuleFileNameA(GetModuleHandleA(NULL), curr_dir, MAX_PATH);
    PathRemoveFileSpecA(curr_dir);

    open_file_struct.cBytes = sizeof(OFSTRUCT);

    HANDLE h_shader_file = (HANDLE)(OpenFile(std::strcat(curr_dir, "\\cuda\\sphere.cu.optixir"), &open_file_struct, OF_READ));
    DWORD file_size = GetFileSize(h_shader_file, nullptr);

    char *shader_data = reinterpret_cast<char *>(std::malloc(sizeof(char) * file_size));
    if (!ReadFile(h_shader_file, shader_data, file_size, nullptr, nullptr))
    {
        DWORD err = GetLastError();
        printf("Read file failed for %s: %d", open_file_struct.szPathName, err);
        goto shutdown;
    }

    CloseHandle(h_shader_file);

    OPTIX_CHECK("create module", optixModuleCreate(optix_dev_ctx, &module_compile_options, &pipeline_compile_options, shader_data, file_size, nullptr, nullptr, &module));

    builtin_is_options.builtinISModuleType = OPTIX_PRIMITIVE_TYPE_SPHERE;
    OPTIX_CHECK("get sphere module", optixBuiltinISModuleGet(optix_dev_ctx, &module_compile_options, &pipeline_compile_options, &builtin_is_options, &sphere_module));

    rg_prog_group_desc.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
    rg_prog_group_desc.raygen.module = module;
    rg_prog_group_desc.raygen.entryFunctionName = "__raygen__rg";
    OPTIX_CHECK("create raygen program group", optixProgramGroupCreate(optix_dev_ctx, &rg_prog_group_desc, 1, &prog_group_options, nullptr, nullptr, &rg_prog_group));

    ms_prog_group_desc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
    ms_prog_group_desc.miss.module = module;
    ms_prog_group_desc.miss.entryFunctionName = "__miss__ms";
    OPTIX_CHECK("crate miss program group", optixProgramGroupCreate(optix_dev_ctx, &ms_prog_group_desc, 1, &prog_group_options, nullptr, nullptr, &ms_prog_group));

    hg_prog_group_desc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    hg_prog_group_desc.hitgroup.moduleCH = module;
    hg_prog_group_desc.hitgroup.entryFunctionNameCH = "__closesthit__ch";
    hg_prog_group_desc.hitgroup.moduleIS = sphere_module;
    OPTIX_CHECK("crate ch program group", optixProgramGroupCreate(optix_dev_ctx, &hg_prog_group_desc, 1, &prog_group_options, nullptr, nullptr, &hg_prog_group));

    pipeline_link_options.maxTraceDepth = 16;
    program_groups[0] = rg_prog_group;
    program_groups[1] = ms_prog_group;
    program_groups[2] = hg_prog_group;

    OPTIX_CHECK("create pipeline", optixPipelineCreate(optix_dev_ctx, &pipeline_compile_options, &pipeline_link_options, program_groups, _countof(program_groups), nullptr, nullptr, &pipeline));

    OPTIX_CHECK("accum stack size", optixUtilAccumulateStackSizes(rg_prog_group, &stack_sizes, pipeline));

    uint32_t direct_callable_stack_size_from_traversal;
    uint32_t direct_callable_stack_size_from_state;
    uint32_t continuation_stack_size;
    OPTIX_CHECK("compute stack sizes", optixUtilComputeStackSizes(&stack_sizes, pipeline_link_options.maxTraceDepth, 0, 0, &direct_callable_stack_size_from_traversal, &direct_callable_stack_size_from_state, &continuation_stack_size));

    CU_CHECK("allocate raygen sbt record", cudaMalloc(reinterpret_cast<void **>(&rg_record_ptr), sizeof(rg_sbt_record)));
    OPTIX_CHECK("raygen pack header", optixSbtRecordPackHeader(rg_prog_group, &rg_sbt));
    std::memcpy(&rg_sbt.data.cam_eye, &cam_eye, sizeof(float3));
    std::memcpy(&rg_sbt.data.pixel_00_loc, &pixel_00_loc, sizeof(float3));
    std::memcpy(&rg_sbt.data.pixel_delta, &pixel_delta, sizeof(float3));
    CU_CHECK("copy raygen sbt record to device", cudaMemcpy(reinterpret_cast<void *>(rg_record_ptr), &rg_sbt, sizeof(rg_sbt_record), cudaMemcpyHostToDevice));

    CU_CHECK("allocate miss sbt record", cudaMalloc(reinterpret_cast<void **>(&ms_record_ptr), sizeof(ms_sbt_record)));
    OPTIX_CHECK("miss pack header", optixSbtRecordPackHeader(ms_prog_group, &ms_sbt));
    ms_sbt.data.r = 128;
    ms_sbt.data.g = 128;
    ms_sbt.data.b = 128;
    CU_CHECK("copy miss sbt record to device", cudaMemcpy(reinterpret_cast<void *>(ms_record_ptr), &ms_sbt, sizeof(ms_sbt_record), cudaMemcpyHostToDevice));

    CU_CHECK("allocate hitgroup sbt record", cudaMalloc(reinterpret_cast<void **>(&hg_record_ptr), sizeof(hg_sbt_record)));
    OPTIX_CHECK("hitgroup pack header", optixSbtRecordPackHeader(hg_prog_group, &hg_sbt));
    CU_CHECK("copy hitgroup sbt record to device", cudaMemcpy(reinterpret_cast<void *>(hg_record_ptr), &hg_sbt, sizeof(hg_sbt_record), cudaMemcpyHostToDevice));

    sbt.raygenRecord = rg_record_ptr;
    sbt.missRecordBase = ms_record_ptr;
    sbt.missRecordStrideInBytes = sizeof(ms_sbt_record);
    sbt.missRecordCount = 1;
    sbt.hitgroupRecordBase = hg_record_ptr;
    sbt.hitgroupRecordStrideInBytes = sizeof(hg_sbt_record);
    sbt.hitgroupRecordCount = 1;

    CU_CHECK("allocate output buffer", cudaMalloc(reinterpret_cast<void **>(&output_buffer), render_height * render_width * sizeof(uchar4)));

    CU_CHECK("create stream", cudaStreamCreate(&stream));

    params.image = reinterpret_cast<uchar4 *>(output_buffer);
    params.image_width = render_width;
    params.image_height = render_height;
    params.handle = gas_handle;

    CU_CHECK("allocate device params", cudaMalloc(reinterpret_cast<void **>(&d_params), sizeof(Params)));
    CU_CHECK("copy params to device", cudaMemcpy(reinterpret_cast<void *>(d_params), &params, sizeof(Params), cudaMemcpyHostToDevice));

    OPTIX_CHECK("launch kernel", optixLaunch(pipeline, stream, d_params, sizeof(Params), &sbt, render_width, render_height, 1));

    CU_CHECK("sync stream", cudaStreamSynchronize(stream));

    CU_CHECK("output buffer to pixels", cudaMemcpy(reinterpret_cast<void *>(pixels), reinterpret_cast<void *>(output_buffer), render_width * render_height * sizeof(uchar4), cudaMemcpyDeviceToHost));

shutdown:
    CU_CHECK("free d params", cudaFree(reinterpret_cast<void *>(d_params)));
    CU_CHECK("free output buffer", cudaFree(output_buffer));
    CU_CHECK("free raygen sbt record", cudaFree(reinterpret_cast<void *>(sbt.raygenRecord)));
    CU_CHECK("free miss sbt record", cudaFree(reinterpret_cast<void *>(sbt.missRecordBase)));
    CU_CHECK("destroy stream", cudaStreamDestroy(stream));
    OPTIX_CHECK("destroy pipeline", optixPipelineDestroy(pipeline));
    OPTIX_CHECK("destroy module", optixModuleDestroy(module));
    OPTIX_CHECK("destroy device context", optixDeviceContextDestroy(optix_dev_ctx));
    printf("============== END OPTIX LOG ==============\n");

    std::free(shader_data);
}