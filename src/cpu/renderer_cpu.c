#include "../common/renderer.h"
#include "ray.h"
#include "color.h"
#include "bbox.h"
#include "../common/utils.h"

#include <Windows.h>

#define NUM_WIDTH_CUTS 20
#define NUM_HEIGHT_CUTS 20 

typedef struct triangle_hit_data
{
	bool is_hit;
	float t;
	vec3 bary_coords;
} triangle_hit_data;

typedef struct bucket_params
{
	size_t x_start;
	size_t y_start;
	size_t x_end;
	size_t y_end;
	size_t render_width;
	size_t render_height;
	size_t num_samples;
	uint8_t* pixels;
	vec3 pixel_00_loc;
	vec3 pixel_delta_u;
	vec3 pixel_delta_v;
} bucket_params;

static bucket_params bps[NUM_WIDTH_CUTS * NUM_HEIGHT_CUTS];
static PTP_WORK works[NUM_WIDTH_CUTS * NUM_HEIGHT_CUTS];
static scene s = { 0 };
static size_t buckets_done = 0;

static float hit_sphere(vec3 center, const float radius, ray ray)
{
	vec3 oc = { 0 };
	glm_vec3_sub(center, ray.org, oc);
	float a = glm_vec3_dot(ray.dir, ray.dir);
	float b = -2.f * glm_vec3_dot(ray.dir, oc);
	float c = glm_vec3_dot(oc, oc) - radius * radius;
	float discrim = b * b - 4.f * a * c;

	if (discrim <= 0.f)
	{
		return discrim;
	}
	else {
		return (-b - sqrtf(discrim)) / (2.f * a);
	}
}

static bool hit_bbox(bbox box, ray ray)
{
	float tmin = (box.bounds[ray.sign[0]][0] - ray.org[0]) * ray.inv_dir[0];
	float tmax = (box.bounds[1 - ray.sign[0]][0] - ray.org[0]) * ray.inv_dir[0];
	float tymin = (box.bounds[ray.sign[1]][1] - ray.org[1]) * ray.inv_dir[1];
	float tymax = (box.bounds[1 - ray.sign[1]][1] - ray.org[1]) * ray.inv_dir[1];

	if ((tmin > tymax) || (tymin > tmax))
	{
		return false;
	}

	if (tymin > tmin)
	{
		tmin = tymin;
	}

	if (tymax < tmax)
	{
		tmax = tymax;
	}

	float tzmin = (box.bounds[ray.sign[2]][2] - ray.org[2]) * ray.inv_dir[2];
	float tzmax = (box.bounds[1 - ray.sign[2]][2] - ray.org[2]) * ray.inv_dir[2];

	if ((tmin > tzmax) || (tzmin > tmax))
	{
		return false;
	}

	return true;
}

static triangle_hit_data hit_triangle(triangle tri, ray ray)
{
	triangle_hit_data hit_data = { 0 };

	vec3 e1 = { 0 }; vec3 e2 = { 0 };
	glm_vec3_sub(tri.positions[1], tri.positions[0], e1);
	glm_vec3_sub(tri.positions[2], tri.positions[0], e2);

	vec3 ray_cross_e2 = { 0 };
	glm_vec3_cross(ray.dir, e2, ray_cross_e2);

	float det = glm_vec3_dot(e1, ray_cross_e2);

	if (det > -0.00001 && det < 0.00001)
		return hit_data;

	float inv_det = 1.f / det;
	vec3 s = { 0 };
	glm_vec3_sub(ray.org, tri.positions[0], s);
	float u = inv_det * glm_vec3_dot(s, ray_cross_e2);

	if (u < 0.f || u > 1.f)
		return hit_data;

	vec3 s_cross_e1 = { 0 };
	glm_vec3_cross(s, e1, s_cross_e1);
	float v = inv_det * glm_vec3_dot(ray.dir, s_cross_e1);

	if (v < 0.f || u + v > 1.f)
		return hit_data;

	float t = inv_det * glm_vec3_dot(e2, s_cross_e1);

	if (t > 0.00001)
	{
		hit_data.is_hit = true;
		hit_data.t = t;

		hit_data.bary_coords[0] = u;
		hit_data.bary_coords[1] = v;
		hit_data.bary_coords[2] = 1 - u - v;

		return hit_data;
	}
	else
	{
		return hit_data;
	}
}

static ray generate_ray(float x, float y, vec3 pixel_00_loc, vec3 pixel_delta_u, vec3 pixel_delta_v, vec3 org)
{
	vec3 offset = { (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX };

	vec3 pixel_center = { 0 };
	vec3 pixel_delta_u_x = { 0 }; vec3 pixel_delta_v_y = { 0 };
	glm_vec3_scale(pixel_delta_u, x, pixel_delta_u_x);
	glm_vec3_scale(pixel_delta_v, y, pixel_delta_v_y);

	vec3 pixel_delta_u_offset = { 0 }; vec3 pixel_delta_v_offset = { 0 };
	glm_vec3_scale(pixel_delta_u, offset[0], pixel_delta_u_offset);
	glm_vec3_scale(pixel_delta_v, offset[1], pixel_delta_v_offset);

	glm_vec3_add(pixel_00_loc, pixel_delta_u_x, pixel_center);
	glm_vec3_add(pixel_center, pixel_delta_v_y, pixel_center);
	glm_vec3_add(pixel_center, pixel_delta_u_offset, pixel_center);
	glm_vec3_add(pixel_center, pixel_delta_v_offset, pixel_center);

	vec3 dir = { 0 };
	glm_vec3_sub(pixel_center, org, dir);
	glm_vec3_normalize(dir);
	
	ray r = ray_create(org, dir);

	return r;
}

static void test_render(ray cam_ray, vec4 sample_color, bool* hit)
{
	vec3 center = { 0.f, 2.f, 0.f };
	float t = hit_sphere(center, 0.5f, cam_ray);
	if (t > 0.f)
	{
		vec4 c = { 0.f, 1.f, 0.f, 0.3f };
		color_blend(c, sample_color, sample_color);
		*hit = true;
	}

	bbox box = {
		.bounds = {
			{-1.0, -1.0, -1.0},
			{1.0, 1.0, 1.0},
		},
	};

	if (hit_bbox(box, cam_ray))
	{
		vec4 c = { 1.f, 0.f, 0.f, 0.3f };
		color_blend(c, sample_color, sample_color);
		*hit = true;
	}

	triangle tri = {
		.positions = {
			{-0.5f, -0.5f, 0 },
			{0.5f, -0.5f, 0 },
			{0.5f, 0.5f, 0 },
		},
	};

	triangle_hit_data hit_data = hit_triangle(tri, cam_ray);
	if (hit_data.is_hit)
	{
		vec4 c = { hit_data.bary_coords[0], hit_data.bary_coords[1], hit_data.bary_coords[2], 0.5f };
		color_blend(c, sample_color, sample_color);
		*hit = true;
	}
}

static void ray_cast(const ray r, vec4 out_color)
{
	bool hit = false;
	float t_min = FLT_MAX;

	for (size_t p = 0; p < s.prims_count; ++p)
	{
		primitive curr_prim = s.prims[p];

		if (hit_bbox(curr_prim.bbox, r))
		{
			for (size_t t = 0; t < curr_prim.tris_count; ++t)
			{
				triangle curr_tri = curr_prim.tris[t];

				triangle_hit_data thd = hit_triangle(curr_tri, r);
				if (thd.is_hit && thd.t < t_min)
				{
					t_min = thd.t;
					vec4 color = { 0.f, r.dir[1] + 1.f * 0.5f, 0.f, 1.f };
					glm_vec4_one(color);
					//glm_vec4(curr_tri.normals[0], color[3], color);
					glm_vec3_copy(thd.bary_coords, color);
					color_blend(color, out_color, out_color);
					//material_get_color(curr_prim.material, r, curr_tri, out_color);
					hit = true;
				}
			}
		}
	}

	if (!hit)
	{
		vec4 c = { 0.2f, 0.2f, 0.2f, 1.f };
		color_blend(c, out_color, out_color);
	}
}

static void CALLBACK render_bucket(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work)
{
	(Instance);    (Work);

	if (Parameter == NULL)
		return;

	bucket_params bp = *(bucket_params*)Parameter;

	for (size_t y = bp.y_start; y < bp.y_end; ++y)
	{
		for (size_t x = bp.x_start; x < bp.x_end; ++x)
		{
			vec4 pixel_color = { 0.0f, 0.0f, 0.0f, 0.0f };

			for (uint8_t n = 0; n < bp.num_samples; ++n)
			{
				vec4 sample_color = { 0 };

				ray cam_ray = generate_ray((float)x, (float)y, bp.pixel_00_loc, bp.pixel_delta_u, bp.pixel_delta_v, s.camera.pos);
				ray_cast(cam_ray, sample_color);

				glm_vec4_add(sample_color, pixel_color, pixel_color);
			}

			glm_vec4_scale(pixel_color, 1.f / bp.num_samples, pixel_color);
			glm_vec4_clamp(pixel_color, 0.f, 1.f);

			bp.pixels[(y * (size_t)bp.render_width * 4) + (x * 4)] = (uint8_t)(pixel_color[0] * 255);
			bp.pixels[(y * (size_t)bp.render_width * 4) + (x * 4) + 1] = (uint8_t)(pixel_color[1] * 255);
			bp.pixels[(y * (size_t)bp.render_width * 4) + (x * 4) + 2] = (uint8_t)(pixel_color[2] * 255);
			bp.pixels[(y * (size_t)bp.render_width * 4) + (x * 4) + 3] = (uint8_t)(pixel_color[3] * 255);
		}
	}

	printf("\rBuckets done: %lld / %d", InterlockedIncrement64(&buckets_done), NUM_WIDTH_CUTS * NUM_HEIGHT_CUTS);
}

void renderer_render_cpu(const size_t render_width, const size_t render_height, const uint8_t num_samples, const char* gltf_path, uint8_t* pixels)
{
	s = scene_create(gltf_path);

	vec3 pixel_00_loc = { 0 }; vec3 pixel_delta_u = { 0 }; vec3 pixel_delta_v = { 0 };
	utils_find_pixel_vecs(s.camera, s.camera.fov, (float)render_width, (float)render_height, pixel_00_loc, pixel_delta_u, pixel_delta_v);

	size_t bucket_width = render_width / NUM_WIDTH_CUTS;
	size_t bucket_height = render_height / NUM_HEIGHT_CUTS;

	for (size_t yc = 0; yc < NUM_HEIGHT_CUTS; ++yc)
	{
		for (size_t xc = 0; xc < NUM_WIDTH_CUTS; ++xc)
		{
			size_t bucket_idx = yc * NUM_WIDTH_CUTS + xc;
			bps[bucket_idx].x_start = bucket_width * xc;
			bps[bucket_idx].y_start = bucket_height * yc;
			bps[bucket_idx].x_end = bps[bucket_idx].x_start + bucket_width;
			bps[bucket_idx].y_end = bps[bucket_idx].y_start + bucket_height;
			bps[bucket_idx].render_width = render_width;
			bps[bucket_idx].render_height = render_height;
			bps[bucket_idx].num_samples = num_samples;
			bps[bucket_idx].pixels = pixels;
			glm_vec3_copy(pixel_00_loc, bps[bucket_idx].pixel_00_loc);
			glm_vec3_copy(pixel_delta_u, bps[bucket_idx].pixel_delta_u);
			glm_vec3_copy(pixel_delta_v, bps[bucket_idx].pixel_delta_v);

			works[bucket_idx] = CreateThreadpoolWork(render_bucket, bps + bucket_idx, NULL);
			SubmitThreadpoolWork(works[bucket_idx]);
		}
	}

	for (size_t yc = 0; yc < NUM_HEIGHT_CUTS; ++yc)
	{
		for (size_t xc = 0; xc < NUM_WIDTH_CUTS; ++xc)
		{
			size_t bucket_idx = yc * NUM_WIDTH_CUTS + xc;

			WaitForThreadpoolWorkCallbacks(works[bucket_idx], FALSE);
			CloseThreadpoolWork(works[bucket_idx]);
		}
	}

	printf("\n");

	scene_destroy(s);
}
