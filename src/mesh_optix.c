#include "mesh_optix.h"
#include "utils.h"

mesh_optix mesh_optix_create(cgltf_node* curr_node, const OptixDeviceContext ctx, const cudaStream_t stream)
{
    mesh_optix m = { 0 };

    utils_get_xform_matrix_for_node(curr_node, m.xform);

    cgltf_mesh* curr_mesh = curr_node->mesh;

    m.prims_count = curr_mesh->primitives_count;
    m.prims = malloc(sizeof(primitive_optix) * m.prims_count);

    for (size_t p = 0; p < m.prims_count; ++p)
    {
        m.prims[p] = primitive_optix_create(curr_mesh->primitives + p, m.xform, ctx, stream);
    }

    return m;
}

void mesh_optix_destroy(mesh_optix m)
{
    if (m.prims != NULL)
    {
        for (size_t p = 0; p < m.prims_count; ++p)
        {
            primitive_optix_destroy(m.prims[p]);
        }

        free(m.prims);
        m.prims = NULL;
        m.prims_count = 0;
    }
}
