#include "bbox.h"

bbox bbox_create(void)
{
    bbox b = {
        .bounds = {
            {FLT_MAX, FLT_MAX, FLT_MAX},
            {-FLT_MAX, -FLT_MAX, -FLT_MAX},
        },
        .center = {
            0.f,0.f,0.f
        },
    };

    return b;
}

bbox bbox_create_with_bounds(vec3 min, vec3 max)
{
    bbox b = { 0 };

    glm_vec3_copy(min, b.bounds[0]);
    glm_vec3_copy(max, b.bounds[1]);
    bbox_calculate_center(&b);

    return b;
}

void bbox_expand_to_vec3(bbox* b, vec3 pt)
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

    bbox_calculate_center(b);
}

void bbox_expand_to_tri(bbox* b, triangle tri)
{
    for (size_t t = 0; t < _countof(tri.positions); ++t)
    {
        bbox_expand_to_vec3(b, tri.positions[t]);
    }
    bbox_calculate_center(b);
}

void bbox_expand_to_bbox(bbox* b, bbox o)
{
    bbox_expand_to_vec3(b, o.bounds[0]);
    bbox_expand_to_vec3(b, o.bounds[1]);
    bbox_calculate_center(b);
}

void bbox_calculate_center(bbox* b)
{
    glm_vec3_add(b->bounds[0], b->bounds[1], b->center);
    glm_vec3_scale(b->center, 0.5f, b->center);
}

bool bbox_contains_vec3(bbox b, vec3 pt)
{
    return b.bounds[0][0] < pt[0] && b.bounds[0][1] < pt[1] && b.bounds[0][2] < pt[2] &&
        b.bounds[1][0] > pt[0] && b.bounds[1][1] > pt[1] && b.bounds[1][2] > pt[2];
}

bool bbox_contains_bbox(bbox b, bbox o)
{
    return b.bounds[0][0] < o.bounds[0][0] && b.bounds[0][1] < o.bounds[0][1] && b.bounds[0][2] < o.bounds[0][2] &&
        b.bounds[1][0] > o.bounds[1][0] && b.bounds[1][1] > o.bounds[1][1] && b.bounds[1][2] > o.bounds[1][2];
}
