#include "optix_renderer.hpp"

#include <Shlwapi.h>

#include <optix.h>
#include <optix_stubs.h>
#include <cuda_runtime.h>

#undef min
#undef max
#include <optix_stack_size.h>
#include <optix_function_table_definition.h>

#include <iostream>
#include <sstream>
#include <vector>

struct Params
{
    uchar4* image;
    unsigned int image_width;
};

struct RaygenData
{
    float r;
    float g;
    float b;
};

struct raygen_sbt_record
{
    __align__(OPTIX_SBT_RECORD_ALIGNMENT) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    RaygenData data;
};

struct miss_sbt_record
{
    __align__(OPTIX_SBT_RECORD_ALIGNMENT) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    int data;
};

static inline void OPTIX_CHECK(const std::string& action, const OptixResult result)
{
    if (result > 0)
    {
        std::stringstream msg;
        msg << "OPTIX ERR [" << result << "]: " << action << '\n';

        std::cerr << msg.str();

        std::exit(result);
    }
}

static inline void CU_CHECK(const std::string& action, const cudaError error)
{
    if (error > 0)
    {
        std::stringstream msg;
        msg << "CUDA ERR [" << error << "]: " << action << '\n';

        std::cerr << msg.str();

        std::exit(error);
    }
}

static inline void CU_CHECK(const std::string& action, const CUresult result)
{
    if (result > 0)
    {
        std::stringstream msg;
        msg << "CUDA ERR [" << result << "]: " << action << '\n';

        std::cerr << msg.str();

        std::exit(result);
    }
}

static void optx_log_cb(unsigned int level, const char* tag, const char* message, void* cb_data)
{
    std::cout << "Level: " << level << ' ' << tag << ' ' << message << '\n';
}

void optx_renderer::render(const uint32_t render_width, const uint32_t render_height, uint8_t* pixels)
{
    std::cout << "============== BEGIN OPTIX LOG ==============\n";
    CU_CHECK("init cuda", cudaFree(nullptr));
    OPTIX_CHECK("init optix", optixInit());

    OptixDeviceContext opx_dev_ctx = 0;
    OptixDeviceContextOptions options = {};
#ifdef DEBUG
    options.logCallbackFunction = optx_log_cb;
    options.logCallbackLevel = 4;
    options.validationMode = OPTIX_DEVICE_CONTEXT_VALIDATION_MODE_ALL;
#endif // DEBUG

    OPTIX_CHECK("create device context", optixDeviceContextCreate(0, &options, &opx_dev_ctx));

    OptixModule module = nullptr;
    OptixPipelineCompileOptions pipeline_compile_options = {};
    OptixModuleCompileOptions module_compile_options = {};
#ifdef DEBUG
    module_compile_options.optLevel = OPTIX_COMPILE_OPTIMIZATION_LEVEL_0;
    module_compile_options.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_FULL;
#endif // DEBUG

    pipeline_compile_options.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_LEVEL_INSTANCING;
    pipeline_compile_options.numPayloadValues = 2;
    pipeline_compile_options.numAttributeValues = 2;
    pipeline_compile_options.exceptionFlags = OPTIX_EXCEPTION_FLAG_STACK_OVERFLOW | OPTIX_EXCEPTION_FLAG_TRACE_DEPTH;
    pipeline_compile_options.pipelineLaunchParamsVariableName = "params";

    char curr_dir[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA(NULL), curr_dir, MAX_PATH);
    PathRemoveFileSpecA(curr_dir);

    OFSTRUCT open_file_struct = {};
    open_file_struct.cBytes = sizeof(OFSTRUCT);

    HANDLE h_shader_file = reinterpret_cast<HANDLE>(OpenFile(std::strcat(curr_dir, "\\cuda\\green.cu.optixir"), &open_file_struct, OF_READ));

    DWORD file_size = GetFileSize(h_shader_file, nullptr);

    std::vector<char> shader_data(file_size);
    if (!ReadFile(h_shader_file, shader_data.data(), file_size, nullptr, nullptr))
    {
        std::cerr << "Could not read shader file " << open_file_struct.szPathName << '\n';
        std::exit(GetLastError());
    }

    CloseHandle(h_shader_file);

    OPTIX_CHECK("create module", optixModuleCreate(opx_dev_ctx, &module_compile_options, &pipeline_compile_options, shader_data.data(), shader_data.size(), nullptr, nullptr, &module));

    OptixProgramGroup raygen_prog_group = nullptr;
    OptixProgramGroupDesc raygen_prog_group_desc = {};
    raygen_prog_group_desc.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
    raygen_prog_group_desc.raygen.module = module;
    raygen_prog_group_desc.raygen.entryFunctionName = "__raygen__draw_solid_color";
    OptixProgramGroupOptions raygen_prog_group_options = {};
    optixProgramGroupCreate(opx_dev_ctx, &raygen_prog_group_desc, 1, &raygen_prog_group_options, nullptr, nullptr, &raygen_prog_group);

    OptixProgramGroup miss_prog_group = nullptr;
    OptixProgramGroupDesc miss_prog_group_desc = {};
    miss_prog_group_desc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
    OptixProgramGroupOptions miss_prog_group_options = {};
    optixProgramGroupCreate(opx_dev_ctx, &miss_prog_group_desc, 1, &miss_prog_group_options, nullptr, nullptr, &miss_prog_group);

    OptixProgramGroup program_groups[] = {
        raygen_prog_group,
        miss_prog_group,
    };

    OptixPipelineLinkOptions pipeline_link_options = {};
    pipeline_link_options.maxTraceDepth = 16;

    OptixPipeline pipeline = nullptr;
    OPTIX_CHECK("create pipeline", optixPipelineCreate(opx_dev_ctx, &pipeline_compile_options, &pipeline_link_options, program_groups, _countof(program_groups), nullptr, nullptr, &pipeline));

    OptixStackSizes stack_sizes = {};
    OPTIX_CHECK("accum stack size", optixUtilAccumulateStackSizes(raygen_prog_group, &stack_sizes, pipeline));

    uint32_t direct_callable_stack_size_from_traversal;
    uint32_t direct_callable_stack_size_from_state;
    uint32_t continuation_stack_size;
    OPTIX_CHECK("compute stack sizes", optixUtilComputeStackSizes(&stack_sizes, pipeline_link_options.maxTraceDepth, 0, 0, &direct_callable_stack_size_from_traversal, &direct_callable_stack_size_from_state, &continuation_stack_size));

    OptixShaderBindingTable sbt = {};

    CUdeviceptr raygen_record_ptr;
    CU_CHECK("allocate raygen sbt record", cudaMalloc(reinterpret_cast<void**>(&raygen_record_ptr), sizeof(raygen_sbt_record)));
    raygen_sbt_record rg_sbt;
    OPTIX_CHECK("raygen pack header", optixSbtRecordPackHeader(raygen_prog_group, &rg_sbt));
    rg_sbt.data = { 0.1,0.2,0.1 };
    CU_CHECK("copy raygen sbt record to device", cudaMemcpy(reinterpret_cast<void*>(raygen_record_ptr), &rg_sbt, sizeof(raygen_sbt_record), cudaMemcpyHostToDevice));

    CUdeviceptr miss_record_ptr;
    CU_CHECK("allocate miss sbt record", cudaMalloc(reinterpret_cast<void**>(&miss_record_ptr), sizeof(miss_sbt_record)));
    miss_sbt_record miss_sbt;
    OPTIX_CHECK("miss pack header", optixSbtRecordPackHeader(miss_prog_group, &miss_sbt));
    CU_CHECK("copy miss sbt record to device", cudaMemcpy(reinterpret_cast<void*>(miss_record_ptr), &miss_sbt, sizeof(miss_sbt_record), cudaMemcpyHostToDevice));

    sbt.raygenRecord = raygen_record_ptr;
    sbt.missRecordBase = miss_record_ptr;
    sbt.missRecordStrideInBytes = sizeof(miss_sbt_record);
    sbt.missRecordCount = 1;

    uchar4* output_buffer = nullptr;

    CU_CHECK("allocate output buffer", cudaMalloc(reinterpret_cast<void**>(&output_buffer), render_height * render_width * sizeof(uchar4)));

    CUstream stream;
    CU_CHECK("create stream", cudaStreamCreate(&stream));

    Params params;
    params.image = reinterpret_cast<uchar4*>(output_buffer);
    params.image_width = render_width;

    CUdeviceptr d_params;
    CU_CHECK("allocate device params", cudaMalloc(reinterpret_cast<void**>(&d_params), sizeof(Params)));
    CU_CHECK("copy params to device", cudaMemcpy(reinterpret_cast<void*>(d_params), &params, sizeof(Params), cudaMemcpyHostToDevice));

    OPTIX_CHECK("launch kernel", optixLaunch(pipeline, stream, d_params, sizeof(Params), &sbt, render_width, render_height, 1));

    CU_CHECK("sync stream", cudaStreamSynchronize(stream));
    CU_CHECK("free d params", cudaFree(reinterpret_cast<void*>(d_params)));

    for (size_t i = 0; i < render_width * render_height * 4; i += 4)
    {
        pixels[i] = std::rand() % 255;
        pixels[i + 1] = std::rand() % 255;
        pixels[i + 2] = std::rand() % 255;
    }

    CU_CHECK("output buffer to pixels", cudaMemcpy(reinterpret_cast<void*>(pixels), reinterpret_cast<void*>(output_buffer), render_width * render_height * sizeof(uchar4), cudaMemcpyDeviceToHost));

    CU_CHECK("free output buffer", cudaFree(output_buffer));
    CU_CHECK("free raygen sbt record", cudaFree(reinterpret_cast<void*>(sbt.raygenRecord)));
    CU_CHECK("free miss sbt record", cudaFree(reinterpret_cast<void*>(sbt.missRecordBase)));
    CU_CHECK("destroy stream", cudaStreamDestroy(stream));
    OPTIX_CHECK("destroy pipeline", optixPipelineDestroy(pipeline));
    OPTIX_CHECK("destroy module", optixModuleDestroy(module));
    OPTIX_CHECK("destroy device context", optixDeviceContextDestroy(opx_dev_ctx));
    std::cout << "============== END OPTIX LOG ==============\n";
}
