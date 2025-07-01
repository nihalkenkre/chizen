#include "renderer.h"
#include "ray.h"
#include "color.h"
#include "bbox.h"

typedef struct triangle_hit_data
{
    bool is_hit;
    float t;
    vec3 bary_coords;
} triangle_hit_data;

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

static triangle_hit_data hit_triangle(vec3 vtxs[], ray ray)
{
    triangle_hit_data hit_data = { 0 };

    vec3 e1 = { 0 }; vec3 e2 = { 0 };
    glm_vec3_sub(vtxs[1], vtxs[0], e1);
    glm_vec3_sub(vtxs[2], vtxs[0], e2);

    vec3 ray_cross_e2 = { 0 };
    glm_vec3_cross(ray.dir, e2, ray_cross_e2);

    float det = glm_vec3_dot(e1, ray_cross_e2);

    if (det > -0.00001 && det < 0.00001)
        return hit_data;

    float inv_det = 1.f / det;
    vec3 s = { 0 };
    glm_vec3_sub(ray.org, vtxs[0], s);
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

static void find_pixel_vecs(camera cam, float viewport_width, float viewport_height, float render_width, float render_height, float focal_length, vec3 pixel_00_loc, vec3 pixel_delta_u, vec3 pixel_delta_v)
{
    vec3 viewport_u = { 0 }; vec3 viewport_v = { 0 };
    glm_vec3_scale(cam.u, viewport_width, viewport_u);
    glm_vec3_scale(cam.v, -viewport_height, viewport_v);

    glm_vec3_scale(viewport_u, 1.f / render_width, pixel_delta_u);
    glm_vec3_scale(viewport_v, 1.f / render_height, pixel_delta_v);

    vec3 image_plane_offset = { 0 };
    glm_vec3_scale(cam.w, focal_length, image_plane_offset);
    vec3 viewport_upper_left = { 0 };
    glm_vec3_sub(cam.pos, image_plane_offset, viewport_upper_left);

    vec3 viewport_u_by_2 = { 0 }; vec3 viewport_v_by_2 = { 0 };
    glm_vec3_scale(viewport_u, 0.5f, viewport_u_by_2);
    glm_vec3_scale(viewport_v, 0.5f, viewport_v_by_2);

    glm_vec3_sub(viewport_upper_left, viewport_u_by_2, viewport_upper_left);
    glm_vec3_sub(viewport_upper_left, viewport_v_by_2, viewport_upper_left);

    vec3 pixel_delta_offset = { 0 };
    glm_vec3_add(pixel_delta_u, pixel_delta_v, pixel_delta_offset);
    glm_vec3_scale(pixel_delta_offset, 0.5f, pixel_delta_offset);

    glm_vec3_add(viewport_upper_left, pixel_delta_offset, pixel_00_loc);
}

void renderer_render(const float render_width, const float render_height, const uint8_t num_samples, scene scene, uint8_t* pixels)
{
    float focal_length = 2.f;
    float theta = scene.camera.fov;
    float h = tanf(theta / 2.f);
    float viewport_height = 2.f * h * focal_length;
    float viewport_width = viewport_height * (render_width / render_height);

    vec3 pixel_00_loc = { 0 }; vec3 pixel_delta_u = { 0 }; vec3 pixel_delta_v = { 0 };
    find_pixel_vecs(scene.camera, viewport_width, viewport_height, render_width, render_height, focal_length, pixel_00_loc, pixel_delta_u, pixel_delta_v);

    for (size_t y = 0; y < (size_t)render_height; ++y)
    {
        for (size_t x = 0; x < (size_t)render_width; ++x)
        {
            vec4 color = { 0.2f, 0.2f, 0.2f, 0.f };

            for (uint8_t n = 0; n < num_samples; ++n)
            {
                vec4 sample_color = { 0 };

                ray cam_ray = generate_ray((float)x, (float)y, pixel_00_loc, pixel_delta_u, pixel_delta_v, scene.camera.pos);

                bool hit = false;

                //vec3 center = { 0.f, 2.f, 0.f };
                //float t = hit_sphere(center, 0.5f, cam_ray);
                //if (t > 0.f)
                //{
                //    vec4 c = { 0.f, 1.f, 0.f, 0.3f };
                //    color_blend(c, sample_color, sample_color);
                //    hit = true;
                //}

                //bbox box = {
                //    .bounds = {
                //        {-1.0, -1.0, -1.0},
                //        {1.0, 1.0, 1.0},
                //    },
                //};

                //if (hit_bbox(box, cam_ray))
                //{
                //    vec4 c = { 1.f, 0.f, 0.f, 0.3f };
                //    color_blend(c, sample_color, sample_color);
                //    hit = true;
                //}

                //vec3 vtxs[] = {
                //    {-0.5f, -0.5f, 0 },
                //    {0.5f, -0.5f, 0 },
                //    {0.5f, 0.5f, 0 },
                //};

                //triangle_hit_data hit_data = hit_triangle(vtxs, cam_ray);
                //if (hit_data.is_hit)
                //{
                //    vec4 c = { hit_data.bary_coords[0], hit_data.bary_coords[1], hit_data.bary_coords[2], 0.5f };
                //    color_blend(c, sample_color, sample_color);
                //    hit = true;
                //}

                for (size_t m = 0; m < scene.meshes_count; ++m)
                {
                    mesh curr_mesh = scene.meshes[m];

                    for (size_t p = 0; p < curr_mesh.prims_count; ++p)
                    {
                        primitive curr_prim = curr_mesh.prims[p];

                        if (hit_bbox(curr_prim.bbox, cam_ray))
                        {
                            vec4 c = { 1.f, 0.f, 0.f, 0.3f };
                            color_blend(c, sample_color, sample_color);
                            hit = true;
                        }
                    }
                }

                if (!hit)
                {
                    vec4 c = { 0.2f, 0.2f, 0.2f, 1.f };
                    color_blend(c, sample_color, sample_color);
                }

                glm_vec4_add(sample_color, color, color);
            }

            glm_vec4_scale(color, 1.f / num_samples, color);
            glm_vec4_clamp(color, 0.f, 1.f);

            pixels[(y * (size_t)render_width * 4) + (x * 4)] = (uint8_t)(color[0] * 255);
            pixels[(y * (size_t)render_width * 4) + (x * 4) + 1] = (uint8_t)(color[1] * 255);
            pixels[(y * (size_t)render_width * 4) + (x * 4) + 2] = (uint8_t)(color[2] * 255);
            pixels[(y * (size_t)render_width * 4) + (x * 4) + 3] = (uint8_t)(color[3] * 255);
        }
    }
}
