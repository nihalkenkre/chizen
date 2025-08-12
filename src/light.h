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
	float position[3];
	float rotation[4];
	float color[3];
	float intensity;
	float range;
	float spot_inner_cone_angle;
	float spot_outer_cone_angle;
	LIGHT_TYPE type;
	CHIZEN_RESULT result;
} light;

light light_create(cgltf_node* curr_node);
void light_destroy(light l);

typedef struct lights
{
	light* lights;
	size_t count;
	CHIZEN_RESULT result;
} lights;

lights lights_create(const cgltf_data* data);
CHIZEN_RESULT lights_destroy(lights l);

