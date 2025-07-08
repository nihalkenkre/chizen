#pragma once

#include "bbox.h"
#include "scene.h"

typedef struct bvh {
    bbox box;

    bbox* children;
    size_t children_count;
} bvh;

bvh bvh_create(const scene scene);
void bvh_destroy(bvh b);