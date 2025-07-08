#pragma once

#include <cglm/include/cglm/cglm.h>
#include "triangle.h"

typedef struct bbox
{
    vec3 bounds[2];
} bbox;

bbox bbox_create(void);
void bbox_expand_to_vec3(bbox* b, vec3 pt);
void bbox_expand_to_tri(bbox* b, triangle tri);

