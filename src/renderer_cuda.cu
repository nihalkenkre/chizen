#include "renderer.h"
#include <cuda_runtime.h>
#include <curand_kernel.h>

static inline void CU_CHECK(const char* action, const cudaError_t result)
{
    if (result > cudaSuccess)
    {
        printf("CUDA ERR %d: %s\nExiting...\n", result, action);
        exit(result);
    }
}

__global__ void static render(const size_t render_width, const size_t render_height, uint8_t* d_pixels)
{
    size_t x = blockIdx.x * blockDim.x + threadIdx.x;
    size_t y = blockIdx.y * blockDim.y + threadIdx.y;

    curandState rand_state = { 0 };

    curand_init(1237, y * render_width + x, 0, &rand_state);

    d_pixels[(y * render_width * 4) + (x * 4)] = ((float)x / render_width * curand_uniform(&rand_state)) * 255;
    d_pixels[(y * render_width * 4) + (x * 4) + 1] = ((float)y / render_height * curand_uniform(&rand_state)) * 255;
    d_pixels[(y * render_width * 4) + (x * 4) + 2] = 0;
    d_pixels[(y * render_width * 4) + (x * 4) + 3] = 255;
}

extern "C" void renderer_render_cuda(const size_t render_width, const size_t render_height, const uint8_t num_samples, scene scene, uint8_t* pixels)
{
    cudaFree(nullptr);

    // max tx * ty = 1024 (max threads per block)
    size_t tx = 16;
    size_t ty = 16;

    dim3 blocks(render_width / tx, render_height / ty, 1);
    dim3 threads(tx, ty, 1);

    void* d_pixels = nullptr;
    CU_CHECK("allocate d_pixels", cudaMalloc(&d_pixels, render_width * render_height * 4));

    render << <blocks, threads >> > ((size_t)render_width, (size_t)render_height, (uint8_t*)d_pixels);

    CU_CHECK("get last error", cudaGetLastError());
    CU_CHECK("cuda sync", cudaDeviceSynchronize());
    CU_CHECK("copy from d_pixels", cudaMemcpy(pixels, d_pixels, render_width * render_height * 4, cudaMemcpyDeviceToHost));

    CU_CHECK("free d_pixels", cudaFree(d_pixels));
}