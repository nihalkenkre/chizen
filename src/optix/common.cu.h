#include <stdint.h>

#include <curand_kernel.h>
#include <cuda_runtime.h>

#include <optix.h>
#include <optix_stubs.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <optix_stack_size.h>
#include <optix_function_table_definition.h>
#include "../utils.h"

typedef struct launch_params
{
    texture* textures;
    material* materials;
    exr_pass* passes;
    size_t passes_count;
    size_t render_width;
    size_t render_height;
    OptixTraversableHandle handle;
} launch_params;

typedef struct ray_gen_record_data
{
    float3 pixel_00_loc;
    float3 pixel_delta_u;
    float3 pixel_delta_v;
    float3 org;
    size_t num_samples;
    curandState* states;
} ray_gen_record_data;

typedef struct ray_gen_record
{
    __align__(OPTIX_SBT_RECORD_ALIGNMENT)
        char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    ray_gen_record_data data;
} ray_gen_record;

typedef struct closest_hit_record_data
{
    float3 DUMMY;
} closest_hit_record_data;

typedef struct closest_hit_record
{
    __align__(OPTIX_SBT_RECORD_ALIGNMENT)
        char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    closest_hit_record_data* data;
} closest_hit_record;

typedef struct miss_record_data
{
    float3 DUMMY;
} miss_record_data;

typedef struct miss_record
{
    __align__(OPTIX_SBT_RECORD_ALIGNMENT)
        char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    miss_record_data* data;
} miss_record;

typedef struct ray
{
    float3 org;
    float3 dir;
    float3 inv_dir;
    uint3 sign;
} ray;
