#include "light.h"
#include <string.h>
#include <cglm/include/cglm/cglm.h>
#include "utils.h"
// #include <sutil/vec_math.h>

light light_create(cgltf_node* curr_node)
{
	cgltf_light* curr_light = curr_node->light;

	mat4 xform = { 0 };
	utils_get_xform_matrix_for_node(curr_node, xform);

	vec4 t = { 0 }; mat4 r = { 0 }; vec3 s = { 0 };
	glm_decompose(xform, t, r, s);
	versor r_quat = { 0 };
	glm_mat4_quat(r, r_quat);

	light l = {
		.intensity = curr_light->intensity,
		.range = curr_light->range,
		.spot_inner_cone_angle = curr_light->spot_inner_cone_angle,
		.spot_outer_cone_angle = curr_light->spot_outer_cone_angle,
	};

	l.position.x = t[0];
	l.position.y = t[1];
	l.position.z = t[2];

	l.rotation.x = r_quat[0];
	l.rotation.y = r_quat[1];
	l.rotation.z = r_quat[2];
	l.rotation.w = r_quat[3];

	l.color.x = curr_light->color[0];
	l.color.y = curr_light->color[1];
	l.color.z = curr_light->color[2];

	switch (curr_light->type)
	{
	case cgltf_light_type_directional:
		l.type = LIGHT_TYPE_DIRECTIONAL;
		break;

	case cgltf_light_type_point:
		l.type = LIGHT_TYPE_POINT;
		break;

	case cgltf_light_type_spot:
		l.type = LIGHT_TYPE_SPOT;
		break;

	default:
		break;
	}

	return l;
}

void light_destroy(light l)
{
}

// light data duplication possible if one light is used for
//	more than one node. But data is small and duplication might
// be better than indexing into light array. (cache miss potential)
lights lights_create(const cgltf_data* data)
{
	lights l = { 0 };

	for (size_t n = 0; n < data->nodes_count; ++n)
	{
		if ((data->nodes + n)->light != NULL)
			++l.count;
	}

	l.lights = calloc(l.count, sizeof(light));
	if (l.lights == NULL)
	{
		printf("calloc failed for lights\n");
		l.result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
		goto cpu_error;
	}

	size_t light_index = 0;
	for (size_t n = 0; n < data->nodes_count; ++n)
	{
		if ((data->nodes + n)->light != NULL)
			l.lights[light_index] = light_create(data->nodes + n);
	}

cpu_error:

	return l;
}

CHIZEN_RESULT lights_destroy(lights l)
{
	CHIZEN_RESULT chi_result = CHIZEN_RESULT_SUCCESS;
	free(l.lights);
	l.lights = NULL;
	l.count = 0;

	return chi_result;
}
