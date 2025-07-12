#pragma once

#include "bbox.h"
#include "primitive.h"

typedef struct accel_node accel_node;


typedef struct accel {
    accel_node* root;
} accel;

accel accel_create(primitive* prims, const size_t prims_count);
void accel_destroy(accel a);
