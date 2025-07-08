#include "renderer.h"

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
#include <optix_micromap.h>

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

void renderer_render_optix(const size_t render_width, const size_t render_height, const uint8_t num_samples, scene scene, uint8_t* pixels)
{
    CU_CHECK("init cuda", cudaFree(nullptr));

    OPTIX_CHECK("optix init", optixInit());
    OptixDeviceContext ctx = nullptr;
    OptixDeviceContextOptions ctx_options = {};
    ctx_options.logCallbackLevel = 4;
    ctx_options.logCallbackFunction = log_cb;
    ctx_options.validationMode = OPTIX_DEVICE_CONTEXT_VALIDATION_MODE_ALL;
    
    OPTIX_CHECK("optix device context create", optixDeviceContextCreate(0, &ctx_options, &ctx));

    void* d_pixels = nullptr;
    CU_CHECK("alloc d_pixels", cudaMalloc(&d_pixels, render_width * render_height * 4));

    cudaStream_t stream = nullptr;
    CU_CHECK("create stream", cudaStreamCreate(&stream));

    OptixInstance io = { 0 };
    //optixAccelBuild(ctx,stream, &accel_options, &build_inputs, _countof(build_inputs), tmp_buffer, tmp_buffer_size, output_buffer, output_buffer_size, )
    //optixAccelComputeMemoryUsage(ctx, )

    CU_CHECK("destroy stream", cudaStreamDestroy(stream));
    CU_CHECK("dealloc d_pixels", cudaFree(d_pixels));
    OPTIX_CHECK("optix device context destroy", optixDeviceContextDestroy(ctx));
}
