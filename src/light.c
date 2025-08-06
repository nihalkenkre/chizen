#include "light.h"
#include <string.h>

light light_create(const cgltf_data* data, const cgltf_light* curr_light)
{
	light l = {
		.intensity = curr_light->intensity,
		.range = curr_light->range,
		.spot_inner_cone_angle = curr_light->spot_inner_cone_angle,
		.spot_outer_cone_angle = curr_light->spot_outer_cone_angle
	};

	memcpy(l.color, curr_light->color, sizeof(l.color));

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
