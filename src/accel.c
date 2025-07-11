#include "accel.h"
#include <string.h>

#define ACCEL_NODE_MAX_PRIM_COUNT 5
#define ACCEL_NODE_MAX_VOLUME 125

struct accel_node {
    struct accel_node* child1;
    struct accel_node* child2;

    primitive** prims;
    size_t prims_count;
    bbox box;
};

static size_t bbox_get_largest_dimension_index(bbox b)
{
    vec3 dims = { 0 };
    glm_vec3_sub(b.bounds[1], b.bounds[0], dims);

    if (dims[0] > dims[1] && dims[0] > dims[2])
        return 0;
    if (dims[1] > dims[0] && dims[1] > dims[2])
        return 1;
    if (dims[2] > dims[0] && dims[2] > dims[1])
        return 2;
}

static void bbox_split_along_index(bbox b, size_t split_idx, bbox* out_bbox1, bbox* out_bbox2)
{
    vec3 split_min = { 0 }, split_max = { 0 };
    if (split_idx == 0) {
        split_min[0] = b.center[0];
        split_min[1] = b.bounds[0][1];
        split_min[2] = b.bounds[0][2];

        split_max[0] = b.center[0];
        split_max[1] = b.bounds[1][1];
        split_max[2] = b.bounds[1][2];
    }
    else if (split_idx == 1) {
        split_min[0] = b.bounds[0][0];
        split_min[1] = b.center[1];
        split_min[2] = b.bounds[0][2];

        split_max[0] = b.bounds[1][0];
        split_max[1] = b.center[1];
        split_max[2] = b.bounds[1][2];
    }
    else if (split_idx == 2) {
        split_min[0] = b.bounds[0][0];
        split_min[1] = b.bounds[0][1];
        split_min[2] = b.center[2];

        split_max[0] = b.bounds[1][0];
        split_max[1] = b.bounds[1][1];
        split_max[2] = b.center[2];
    }

    *out_bbox1 = bbox_create_with_bounds(b.bounds[0], split_max);
    *out_bbox2 = bbox_create_with_bounds(split_min, b.bounds[1]);
}

accel_node* accel_create_node(bbox b, primitive** prims, const size_t prims_count)
{
    accel_node* a = malloc(sizeof(accel_node));
    memset(a, 0, sizeof(accel_node));

    glm_vec3_copy(b.bounds[0], a->box.bounds[0]);
    glm_vec3_copy(b.bounds[1], a->box.bounds[1]);
    a->prims = malloc(sizeof(primitive*) * prims_count);

    for (size_t p = 0; p < prims_count; ++p)
    {
        if (bbox_contains_vec3(a->box, (*prims)[p].bbox.center))
        {
            a->prims[a->prims_count++] = *prims + p;
        }
    }

    if (a->prims_count >= ACCEL_NODE_MAX_PRIM_COUNT)
    {
        size_t split_dim = bbox_get_largest_dimension_index(a->box);
        bbox child1_bbox = { 0 }, child2_bbox = { 0 };
        bbox_split_along_index(a->box, split_dim, &child1_bbox, &child2_bbox);
        a->child1 = accel_create_node(child1_bbox, a->prims, a->prims_count);
        a->child2 = accel_create_node(child2_bbox, a->prims, a->prims_count);
        free(a->prims);
        a->prims_count = 0;
    }

    return a;
}

accel accel_create(primitive* prims, const size_t prims_count)
{
    accel a = { 0 };

    bbox b = bbox_create();

    for (size_t p = 0; p < prims_count; ++p)
    {
        bbox_expand_to_bbox(&b, prims[p].bbox);
    }

    a.root = accel_create_node(b, &prims, prims_count);

    return a;
}

void accel_destroy(accel a)
{
}
