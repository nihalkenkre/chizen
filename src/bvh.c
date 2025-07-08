#include "bvh.h"

#define BVH_NODE_DISTANCE_THRESHOLD 5

bvh bvh_create(const scene scene)
{
    bvh b = { 0 };

    //for (size_t m = 0; m < scene.meshes_count; ++m)
    //{
    //    mesh curr_mesh = scene.meshes[m];
    //    for (size_t p = 0; p < curr_mesh.prims_count; ++p)
    //    {
    //        primitive curr_prim = curr_mesh.prims[p];
    //    }
    //}

    return b;
}

void bvh_destroy(bvh b)
{

}
