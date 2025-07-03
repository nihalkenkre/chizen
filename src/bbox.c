#include "bbox.h"

bbox bbox_create(void)
{
    bbox b = {
        .bounds = {
            {FLT_MAX, FLT_MAX, FLT_MAX},
            {-FLT_MAX, -FLT_MAX, -FLT_MAX},
        },
    };

    return b;
}

void bbox_expand_to(bbox* b, vec3 pt)
{
    if (pt[0] < b->bounds[0][0])
        b->bounds[0][0] = pt[0];
    if (pt[1] < b->bounds[0][1])
        b->bounds[0][1] = pt[1];
    if (pt[2] < b->bounds[0][2])
        b->bounds[0][2] = pt[2];

    if (pt[0] > b->bounds[1][0])
        b->bounds[1][0] = pt[0];
    if (pt[1] > b->bounds[1][1])
        b->bounds[1][1] = pt[1];
    if (pt[2] > b->bounds[1][2])
        b->bounds[1][2] = pt[2];
}
