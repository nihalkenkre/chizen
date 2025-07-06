#include "mesh.h"
#include "utils.h"

mesh mesh_create(cgltf_node* mesh_node)
{
    mesh m = { 0 };

    get_xform_matrix_for_node(mesh_node, m.xform);

    cgltf_mesh* curr_mesh = mesh_node->mesh;

    m.prims_count = curr_mesh->primitives_count;
    m.prims = malloc(sizeof(primitive) * m.prims_count);

    for (size_t p = 0; p < m.prims_count; ++p)
    {
        m.prims[p] = primitive_create(curr_mesh->primitives + p, m.xform);
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
    }
}
