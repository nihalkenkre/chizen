#pragma once

#include <cglm/include/cglm/cglm.h>

typedef struct bbox
{
    vec3 bounds[2];
} bbox;

#ifdef __cplusplus
extern "C"
{
#endif
    bbox bbox_create(void);
    void bbox_expand_to(bbox* b, vec3 pt);

#ifdef __cplusplus
}

#endif // __cplusplus

