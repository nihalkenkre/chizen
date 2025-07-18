#include "mesh.h"
#include "utils.h"

#include <string.h>

mesh mesh_create(cgltf_node* curr_node, const OptixDeviceContext ctx, const cudaStream_t stream)
{
	mesh m = { 0 };

	utils_get_xform_matrix_for_node(curr_node, m.xform);

	cgltf_mesh* curr_mesh = curr_node->mesh;

	m.prims_count = curr_mesh->primitives_count;
	m.prims = malloc(sizeof(primitive) * m.prims_count);
	m.instances = malloc(sizeof(OptixInstance) * m.prims_count);
	memset(m.instances, 0, sizeof(OptixInstance) * m.prims_count);

	unsigned int inst_flags = OPTIX_BUILD_FLAG_NONE;
	mat4 xform_transposed = { 0 };
	glm_mat4_transpose_to(m.xform, xform_transposed);
	for (size_t p = 0; p < m.prims_count; ++p)
	{
		m.prims[p] = primitive_create(curr_mesh->primitives + p, ctx, stream);
		m.instances[p].flags = inst_flags;
		m.instances[p].visibilityMask = 0xFF;
		m.instances[p].traversableHandle = m.prims[p].gas_hnd;
		memcpy(m.instances[p].transform, xform_transposed, sizeof(float) * 12);
	}

	return m;
}

void mesh_destroy(mesh m)
{
	if (m.prims != NULL)
	{
		for (size_t p = 0; p < m.prims_count; ++p)
		{
			primitive_destroy(m.prims[p]);
		}

		free(m.prims);
		m.prims = NULL;
		m.prims_count = 0;
	}

	free(m.instances);
	m.instances = NULL;
}
