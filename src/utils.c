#include "utils.h"

void utils_get_xform_matrix_for_node(cgltf_node* node, mat4 xform)
{
    if (node->has_matrix)
    {
        glm_mat4_make(node->matrix, xform);
    }
    else
    {
        glm_mat4_identity(xform);

        if (node->has_translation)
        {
            glm_translate(xform, node->translation);
        }

        if (node->has_rotation)
        {
            vec3 axis = { 0 };
            glm_quat_axis(node->rotation, axis);
            glm_rotate(xform, glm_quat_angle(node->rotation), axis);
        }

        if (node->has_scale)
        {
            glm_scale(xform, node->scale);
        }
    }
}

void utils_find_pixel_vecs(camera cam, float fov, float render_width, float render_height, vec3 out_pixel_00_loc, vec3 out_pixel_delta_u, vec3 out_pixel_delta_v)
{
	float focal_length = 2.f;
	float theta = fov;
	float h = tanf(theta / 2.f);
	float viewport_height = 2.f * h * focal_length;
	float viewport_width = viewport_height * (render_width / render_height);

	vec3 viewport_u = { 0 }; vec3 viewport_v = { 0 };
	glm_vec3_scale(cam.u, viewport_width, viewport_u);
	glm_vec3_scale(cam.v, -viewport_height, viewport_v);

	glm_vec3_scale(viewport_u, 1.f / render_width, out_pixel_delta_u);
	glm_vec3_scale(viewport_v, 1.f / render_height, out_pixel_delta_v);

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
	glm_vec3_add(out_pixel_delta_u, out_pixel_delta_v, pixel_delta_offset);
	glm_vec3_scale(pixel_delta_offset, 0.5f, pixel_delta_offset);

	glm_vec3_add(viewport_upper_left, pixel_delta_offset, out_pixel_00_loc);
}
