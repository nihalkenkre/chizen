#include "helpers.cuh"
#include "../common.h"

#include <cuda_runtime.h>
#include <curand_kernel.h>

#include <cstdio>
#include <ctime>


__global__ void init_random_states_kernel(void* states, size_t seed, uint32_t render_width, uint32_t render_height)
{
	size_t x = blockIdx.x * blockDim.x + threadIdx.x;
	size_t y = blockIdx.y * blockDim.y + threadIdx.y;
		
	if (x > render_width || y > render_height)
		return;

	size_t pixel_idx = y * render_width + x;

	curand_init(pixel_idx + seed, 0, 0, ((curandState*)states) + pixel_idx);
}

void generate_random_states(void* states, uint32_t render_width, uint32_t render_height, cudaStream_t stream)
{
	// tx * ty < 1024 (max threads per block)
	uint32_t tx = min(render_width, 32);
	uint32_t ty = min(render_height, 32);

	dim3 blocks((render_width / tx) + 1, (render_height / ty) + 1);
	dim3 threads(tx, ty);

	srand(NULL);
	init_random_states_kernel<<<blocks, threads, 0, stream>>>(states, rand(), render_width, render_height);
}

__global__ void avg_ld_pixels_kernel(exr_pass* passes, uint32_t passes_count, uint32_t render_width, uint32_t render_height, uint32_t samples_count)
{
	for (size_t p = 0; p < passes_count; ++p)
	{
		size_t x = blockIdx.x * blockDim.x + threadIdx.x;
		size_t y = blockIdx.y * blockDim.y + threadIdx.y;
		
		if (x > render_width || y > render_height)
			continue;

		size_t pixel_idx = (y * render_width * passes[p].layer.num_channels) + (x * passes[p].layer.num_channels);
		
		if (passes[p].layer.type == EXR_LAYER_TYPE_DIFFUSE ||
			passes[p].layer.type == EXR_LAYER_TYPE_SPECULAR ||
			passes[p].layer.type == EXR_LAYER_TYPE_FINALCOLOR
		)
		{
			passes[p].d_pixels[pixel_idx] /= samples_count;
			passes[p].d_pixels[pixel_idx + 1] /= samples_count;
			passes[p].d_pixels[pixel_idx + 2] /= samples_count;
			passes[p].d_pixels[pixel_idx + 3] /= samples_count;

			passes[p].d_pixels[pixel_idx] = powf(passes[p].d_pixels[pixel_idx], 0.454545f);
			passes[p].d_pixels[pixel_idx + 1] = powf(passes[p].d_pixels[pixel_idx + 1], 0.454545f);
			passes[p].d_pixels[pixel_idx + 2] = powf(passes[p].d_pixels[pixel_idx + 2], 0.454545f);
		}
	}
}

void avg_ld_pixels(void* passes, uint32_t passes_count, uint32_t render_width, uint32_t render_height, uint32_t samples_count, cudaStream_t stream)
{
	// tx * ty < 1024 (max threads per block)
	uint32_t tx = min(render_width, 32);
	uint32_t ty = min(render_height, 32);

	dim3 blocks((render_width / tx) + 1, (render_height / ty) + 1);
	dim3 threads(tx, ty);

	avg_ld_pixels_kernel<<<blocks, threads, 0, stream>>>((exr_pass*)passes, passes_count, render_width, render_height, samples_count);
}