#include "renderer.h"
#include "ray.h"
#include "utils.h"

#include <cuda_runtime.h>
#include <curand_kernel.h>

static inline void __device__ __host__ CU_CHECK(const char* action, const cudaError_t result)
{
	if (result > cudaSuccess)
	{
		printf("CUDA ERR %d: %s\nExiting...\n", result, action);
	}
}

typedef struct ray_cu
{
	float3 org;
	float3 dir;
	float3 inv_dir;
	uint3 sign;
} ray_cu;

__device__ static ray_cu ray_cu_create(float3 org, float3 dir)
{
	ray_cu r = { 0 };
	r.org = org;
	r.dir = dir;
	r.inv_dir = { 1.f / dir.x, 1.f / dir.y, 1.f / dir.z };
	r.sign = { r.inv_dir.x < 0.f, r.inv_dir.y < 0.f, r.inv_dir.z < 0.f };

	return r;
}

__device__ float3 static float3_add(float3 a, float3 b)
{
	return { a.x + b.x, a.y + b.y, a.z + b.z };
}

__device__ float3 static float3_scale(float3 v, float s)
{
	return { v.x * s, v.y * s, v.z * s };
}

__device__ float static float3_dot(float3 a, float3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

__device__ float3 static float3_sub(float3 a, float3 b)
{
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}

__device__ float3 static float3_normalize(float3 v)
{
	float mag = sqrtf(float3_dot(v, v));

	return float3_scale(v, 1.f / mag);
}

__device__ static float3 ray_cu_pt_at(ray_cu r, const float t)
{
	float3 pt = float3_scale(r.dir, t);
	return float3_add(r.org, pt);
}

__device__ void static hit_triangle(triangle tri, ray ray)
{

}

__device__ float static hit_sphere(float3 center, const float radius, ray_cu ray)
{
	float3 oc = float3_sub(center, ray.org);
	float a = float3_dot(ray.dir, ray.dir);
	float b = -2.f * float3_dot(ray.dir, oc);
	float c = float3_dot(oc, oc) - radius * radius;
	float discrim = b * b - 4.f * a * c;

	if (discrim <= 0.f)
		return discrim;
	else
		return (-b - sqrtf(discrim)) / (2.f * a);
}

__device__ ray_cu static generate_ray(curandState rand_state, const size_t x, const size_t y, float3 pixel_00_loc, float3 pixel_delta_u, float3 pixel_delta_v, float3 org)
{
	float3 offset = { curand_uniform(&rand_state), curand_uniform(&rand_state), curand_uniform(&rand_state) };

	float3 pixel_delta_u_x = float3_scale(pixel_delta_u, (float)x);
	float3 pixel_delta_v_y = float3_scale(pixel_delta_v, (float)y);

	float3 pixel_delta_u_offset = float3_scale(pixel_delta_u, offset.x);
	float3 pixel_delta_v_offset = float3_scale(pixel_delta_v, offset.y);

	float3 pixel_center = float3_add(pixel_00_loc, pixel_delta_u_x);
	pixel_center = float3_add(pixel_center, pixel_delta_v_y);
	pixel_center = float3_add(pixel_center, pixel_delta_u_offset);
	pixel_center = float3_add(pixel_center, pixel_delta_v_offset);

	float3 dir = float3_normalize(float3_sub(pixel_center, org));

	return ray_cu_create(org, dir);
}

__global__ void static render(const uint8_t num_samples, const size_t render_width, const size_t render_height, float3 pixel_00_loc, float3 pixel_delta_u, float3 pixel_delta_v, scene scene, uint8_t* d_pixels)
{
	size_t x = blockIdx.x * blockDim.x + threadIdx.x;
	size_t y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x > render_width || y > render_height)
		return;

	curandState rand_state = { 0 };
	curand_init(1237, y * render_width + x, 0, &rand_state);

	//for (uint8_t i = 0; i < num_samples; ++i)
	//{
	ray_cu r = generate_ray(rand_state, x, y, pixel_00_loc, pixel_delta_u, pixel_delta_v, { scene.camera.pos[0], scene.camera.pos[1], scene.camera.pos[2] });
	//}

	d_pixels[(y * render_width * 4) + (x * 4)] = 0;// ((float)x / render_width * curand_uniform(&rand_state)) * 255;
	d_pixels[(y * render_width * 4) + (x * 4) + 1] = (uint8_t)((r.dir.y + 1.0 * 0.5) * 255.f);// ((float)y / render_height * curand_uniform(&rand_state)) * 255;
	d_pixels[(y * render_width * 4) + (x * 4) + 2] = 0;
	d_pixels[(y * render_width * 4) + (x * 4) + 3] = 255;
}

extern "C" void renderer_render_cuda(const size_t render_width, const size_t render_height, const uint8_t num_samples, const char* gltf_path, uint8_t* pixels)
{
	scene s = scene_create(gltf_path);
	cudaFree(nullptr);

	// max tx * ty = 1024 (max threads per block)
	size_t tx = 16;
	size_t ty = 16;

	dim3 blocks((unsigned int)(render_width / tx) + 1, (unsigned int)(render_height / ty) + 1, 1);
	dim3 threads((unsigned int)tx, (unsigned int)ty, 1);

	vec3 pixel_00_loc = { 0 }; vec3 pixel_delta_u = { 0 }; vec3 pixel_delta_v = { 0 };
	utils_find_pixel_vecs(s.camera, s.camera.fov, (float)render_width, (float)render_height, pixel_00_loc, pixel_delta_u, pixel_delta_v);

	void* d_pixels = nullptr;
	CU_CHECK("allocate d_pixels", cudaMalloc(&d_pixels, render_width * render_height * 4));

	render << <blocks, threads >> > (num_samples, (size_t)render_width, (size_t)render_height,
		{ pixel_00_loc[0], pixel_00_loc[1], pixel_00_loc[2] },
		{ pixel_delta_u[0], pixel_delta_u[1], pixel_delta_u[2] },
		{ pixel_delta_v[0], pixel_delta_v[1], pixel_delta_v[2] }, s, (uint8_t*)d_pixels);

	CU_CHECK("get last error", cudaGetLastError());
	CU_CHECK("cuda sync", cudaDeviceSynchronize());
	CU_CHECK("copy from d_pixels", cudaMemcpy(pixels, d_pixels, render_width * render_height * 4, cudaMemcpyDeviceToHost));

	CU_CHECK("free d_pixels", cudaFree(d_pixels));
	scene_destroy(s);
}