#include "mesh.h"
#include "utils.h"

#include <string.h>

mesh mesh_create(const cgltf_data* data, cgltf_mesh* curr_mesh, const OptixDeviceContext ctx, const cudaStream_t stream)
{
	mesh m = { 0 };

	m.prims_count = curr_mesh->primitives_count;
	m.prims = malloc(sizeof(primitive) * m.prims_count);
	for (size_t p = 0; p < m.prims_count; ++p)
	{
		m.prims[p] = primitive_create(data, curr_mesh->primitives + p, ctx, stream);
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
}
