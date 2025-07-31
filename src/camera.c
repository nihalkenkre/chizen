#include "camera.h"
#include "utils.h"

camera camera_create_from_gltf(cgltf_node* camera_node)
{
	camera c = {
		.fov = camera_node->camera->data.perspective.yfov,
		.znear = camera_node->camera->data.perspective.znear,
		.zfar = camera_node->camera->data.perspective.has_zfar ? camera_node->camera->data.perspective.zfar : 1000.f,
		.aspect_ratio = camera_node->camera->data.perspective.has_aspect_ratio ? camera_node->camera->data.perspective.aspect_ratio : 16.f / 9.f,
	};

	mat4 xform = { 0 };
	utils_get_xform_matrix_for_node(camera_node, xform);

	vec4 t = { 0 }; mat4 r = { 0 }; vec3 s = { 0 };
	glm_decompose(xform, t, r, s);

	vec3 t3 = { t[0], t[1], t[2] };
	glm_vec3_copy(t3, c.pos);
	glm_vec3_rotate_m4(r, GLM_XUP, c.u);
	glm_vec3_rotate_m4(r, GLM_YUP, c.v);
	glm_vec3_rotate_m4(r, GLM_ZUP, c.w);

	return c;
}

camera camera_create_default(void)
{
	camera c = {
		.fov = 0.4f,
		.znear = 0.1f,
		.zfar = 1000.f,
		.aspect_ratio = 16.f / 9.f,
	};

	mat4 xform = { 0 };
	glm_mat4_identity(xform);
	vec3 eye = { 10, 10, 10 };
	vec3 center = { 0, 0, 0 };
	vec3 up = { 0, 1, 0 };

	glm_lookat(eye, center, up, xform);

	vec4 t = { 0 }; mat4 r = { 0 }; vec3 s = { 0 };
	glm_decompose(xform, t, r, s);

	vec3 t3 = { t[0], t[1], t[2] };
	glm_vec3_copy(t3, c.pos);
	glm_vec3_rotate_m4(r, GLM_XUP, c.u);
	glm_vec3_rotate_m4(r, GLM_YUP, c.v);
	glm_vec3_rotate_m4(r, GLM_ZUP, c.w);

	return c;
}

void camera_destroy(camera c)
{
}
