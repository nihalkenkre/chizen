#include "renderer.h"
#include "ray.h"
#include "color.h"
#include "bbox.h"

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

void renderer_render(const float render_width, const float render_height, const uint8_t num_samples, scene scene, uint8_t* pixels)
{
    float focal_length = 2.f;
    float theta = scene.camera.fov;
    float h = tanf(theta / 2.f);
    float viewport_height = 2.f * h * focal_length;
    float viewport_width = viewport_height * (render_width / render_height);

    vec3 viewport_u = { 0 }; vec3 viewport_v = { 0 };
    glm_vec3_scale(scene.camera.u, viewport_width, viewport_u);
    glm_vec3_scale(scene.camera.v, -viewport_height, viewport_v);

    vec3 pixel_delta_u = { 0 }; vec3 pixel_delta_v = { 0 };
    glm_vec3_scale(viewport_u, 1.f / render_width, pixel_delta_u);
    glm_vec3_scale(viewport_v, 1.f / render_height, pixel_delta_v);

    vec3 image_plane_offset = { 0 };
    glm_vec3_scale(scene.camera.w, focal_length, image_plane_offset);
    vec3 viewport_upper_left = { 0 };
    glm_vec3_sub(scene.camera.pos, image_plane_offset, viewport_upper_left);

    vec3 viewport_u_by_2 = { 0 }; vec3 viewport_v_by_2 = { 0 };
    glm_vec3_scale(viewport_u, 0.5f, viewport_u_by_2);
    glm_vec3_scale(viewport_v, 0.5f, viewport_v_by_2);

    glm_vec3_sub(viewport_upper_left, viewport_u_by_2, viewport_upper_left);
    glm_vec3_sub(viewport_upper_left, viewport_v_by_2, viewport_upper_left);

    vec3 pixel_delta_offset = { 0 };
    glm_vec3_add(pixel_delta_u, pixel_delta_v, pixel_delta_offset);
    glm_vec3_scale(pixel_delta_offset, 0.5f, pixel_delta_offset);

    vec3 pixel_00_loc = { 0 };
    glm_vec3_add(viewport_upper_left, pixel_delta_offset, pixel_00_loc);

    for (size_t y = 0; y < (size_t)render_height; ++y)
    {
        for (size_t x = 0; x < (size_t)render_width; ++x)
        {
            vec4 color = { 0.2f, 0.2f, 0.2f, 0.f };

            for (uint8_t n = 0; n < num_samples; ++n)
            {
                vec4 sample_color = { 0 };

                vec3 offset = { (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX };

                vec3 pixel_center = { 0 };
                vec3 pixel_delta_u_x = { 0 }; vec3 pixel_delta_v_y = { 0 };
                glm_vec3_scale(pixel_delta_u, (float)x, pixel_delta_u_x);
                glm_vec3_scale(pixel_delta_v, (float)y, pixel_delta_v_y);

                vec3 pixel_delta_u_offset = { 0 }; vec3 pixel_delta_v_offset = { 0 };
                glm_vec3_scale(pixel_delta_u, offset[0], pixel_delta_u_offset);
                glm_vec3_scale(pixel_delta_v, offset[1], pixel_delta_v_offset);

                glm_vec3_add(pixel_00_loc, pixel_delta_u_x, pixel_center);
                glm_vec3_add(pixel_center, pixel_delta_v_y, pixel_center);
                glm_vec3_add(pixel_center, pixel_delta_u_offset, pixel_center);
                glm_vec3_add(pixel_center, pixel_delta_v_offset, pixel_center);

                vec3 ray_direction = { 0 };
                glm_vec3_sub(pixel_center, scene.camera.pos, ray_direction);
                glm_vec3_normalize(ray_direction);

                ray r = ray_create(scene.camera.pos, ray_direction);

                bool hit = false;

                vec3 center = { 0.f, 2.f, 0.f };
                float t = hit_sphere(center, 0.5f, r);
                if (t > 0.f)
                {
                    vec4 c = { 0.f, 1.f, 0.f, 0.3f };
                    color_blend(c, sample_color, sample_color);
                    hit = true;
                }

                bbox box = {
                    .bounds =
                    {
                        {-1.0, -1.0, -1.0},
                        {1.0, 1.0, 1.0},
                    },
                };

                if (hit_bbox(box, r))
                {
                    vec4 c = { 1.f, 0.f, 0.f, 0.3f };
                    color_blend(c, sample_color, sample_color);
                    hit = true;
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
