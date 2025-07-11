#pragma once

#include "bbox.h"
#include "material.h"
#include "triangle.h"
#include <cgltf/cgltf.h>

typedef struct primitive
{
    //vec3* positions;
    //size_t positions_count;

    //vec3* normals;
    //size_t normals_count;

    //vec2* uvs;
    //size_t uvs_count;

    //uint32_t* indices;
    //size_t indices_count;

    triangle* tris;
    size_t tris_count;

    material material;
    bbox bbox;

    unsigned long long gas_hnd;
} primitive;

primitive primitive_create(const cgltf_primitive* primitive, mat4 node_xform);
void primitive_destroy(primitive p);
