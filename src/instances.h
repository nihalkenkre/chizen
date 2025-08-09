#pragma once

#include "mesh.h"
#include "error.h"

#include <optix.h>
#include <cgltf/cgltf.h>

typedef struct instance
{
	OptixInstance instance;
	size_t mesh_index;
	CHIZEN_RESULT result;
} instance;

instance instance_create(const cgltf_data* gltf_data, cgltf_node* curr_node, const size_t mesh_idx, const size_t sbt_offset, const OptixTraversableHandle gas_hnd);

typedef struct instances
{
	OptixInstance* instances;
	size_t count;
	CHIZEN_RESULT result;
} instances;


instances instances_create(const cgltf_data* gltf_data, mesh* meshes);
void instances_destroy(instances i);
