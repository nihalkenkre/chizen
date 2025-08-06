#pragma once

#include <cgltf/cgltf.h>
#include "error.h"

typedef enum LIGHT_TYPE
{
	LIGHT_TYPE_DIRECTIONAL,
	LIGHT_TYPE_POINT,
	LIGHT_TYPE_SPOT,
} LIGHT_TYPE;

typedef struct light
{
	float color[3];
	float intensity;
	float range;
	float spot_inner_cone_angle;
	float spot_outer_cone_angle;
	LIGHT_TYPE type;
	CHIZEN_RESULT result;
} light;

light light_create(const cgltf_data* data, const cgltf_light* curr_light);
void light_destroy(light l);
