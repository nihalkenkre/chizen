#include "common.h"


extern "C" __constant__ launch_params lp;

extern "C"  __global__ void __raygen__rg()
{
	uint3 launch_index = optixGetLaunchIndex();
	ray_gen_record_data* rg_data = (ray_gen_record_data*)optixGetSbtDataPointer();

	curandState rand_state = { 0 };
	curand_init(1237, launch_index.y * lp.render_width + launch_index.x, 0, &rand_state);

	ray_cu r = generate_ray(rand_state, launch_index.x, launch_index.y, rg_data->pixel_00_loc, rg_data->pixel_delta_u, rg_data->pixel_delta_v, rg_data->org);

	size_t pixel_idx = (launch_index.y * lp.render_width * 4) + (launch_index.x * 4);
	lp.pixels[pixel_idx] = 0;// (uint8_t)((float)launch_index.x / (float)lp.render_width * 255);
	lp.pixels[pixel_idx + 1] = (uint8_t)((r.dir.y + 1.f * 0.5f) * 255); (uint8_t)((float)launch_index.y / (float)lp.render_height * 255);
	lp.pixels[pixel_idx + 2] = 0;
	lp.pixels[pixel_idx + 3] = 255;
}

extern "C" __global__ void __miss__ms()
{
	miss_record_data* ms_data = (miss_record_data*)optixGetSbtDataPointer();
}