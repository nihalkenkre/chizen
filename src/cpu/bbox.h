#pragma once

#include <cglm/include/cglm/cglm.h>
#include "../cpu/triangle.h"

typedef struct bbox
{
    vec3 bounds[2];
    vec3 center;
} bbox;

bbox bbox_create(void);
bbox bbox_create_with_bounds(vec3 min, vec3 max);

void bbox_calculate_center(bbox* b);

void bbox_expand_to_vec3(bbox* b, vec3 pt);
void bbox_expand_to_tri(bbox* b, triangle tri);
void bbox_expand_to_bbox(bbox* b, bbox o);

bool bbox_contains_vec3(bbox b, vec3 pt);
bool bbox_contains_bbox(bbox b, bbox o);

