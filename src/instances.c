#include "instances.h"
#include "utils.h"

#include <string.h>
#include <cglm/include/cglm/cglm.h>

instance instance_create(const cgltf_data* gltf_data, cgltf_node* curr_node, const size_t mesh_idx, const size_t sbt_offset, const OptixTraversableHandle gas_hnd)
{
	instance i = {
		.instance = {
			.sbtOffset = (unsigned int)sbt_offset,
			.traversableHandle = gas_hnd,
			.visibilityMask = 0xFF,
		},
		.mesh_index = mesh_idx,
	};

	mat4 xform = { 0 };
	utils_get_xform_matrix_for_node(curr_node, xform);
	mat4 xform_xposed = { 0 };
	glm_mat4_transpose_to(xform, xform_xposed);
	memcpy(i.instance.transform, xform_xposed, sizeof(float) * 12);

	return i;
}

instances instances_create(const cgltf_data* gltf_data, mesh* meshes)
{
	instances i = { 0 };

	for (size_t n = 0; n < gltf_data->nodes_count; ++n)
	{
		if (gltf_data->nodes[n].mesh != NULL)
			++i.count;
	}

	size_t instances_size = sizeof(OptixInstance) * i.count;
	i.instances = calloc(1, instances_size);
	if (i.instances == NULL)
	{
		printf("calloc error instances\n");
		i.result = CHIZEN_RESULT_NO_MEMORY_ALLOCED;
		goto shutdown;
	}

	size_t indices_size = sizeof(size_t) * i.count;

	size_t instance_idx = 0;
	size_t sbt_offset = 0;

	for (size_t n = 0; n < gltf_data->nodes_count; ++n)
	{
		if (gltf_data->nodes[n].mesh == NULL)
			continue;

		cgltf_node* curr_node = gltf_data->nodes + n;
		mat4 xform = { 0 };
		utils_get_xform_matrix_for_node(curr_node, xform);
		mat4 xform_xposed = { 0 };
		glm_mat4_transpose_to(xform, xform_xposed);

		size_t mesh_index = cgltf_mesh_index(gltf_data, curr_node->mesh);
		(i.instances + instance_idx)->visibilityMask = 0xFF;
		(i.instances + instance_idx)->traversableHandle = (meshes + mesh_index)->gas_hnd;
		memcpy((i.instances + instance_idx)->transform, xform_xposed, sizeof(float) * 12);

		for (size_t mesh_idx = 0; mesh_idx < mesh_index; ++mesh_idx)
		{
			(i.instances + instance_idx)->sbtOffset += (unsigned int)meshes[mesh_idx].prims_count;
		}

		++instance_idx;
	}

shutdown:
	return i;
}

void instances_destroy(instances i)
{
	free(i.instances);
	i.count = 0;
}
