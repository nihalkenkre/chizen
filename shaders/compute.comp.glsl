#version 460

#define UINT32_MAX 0xFFFFFFFF

layout(set=0, binding=0, rgba32f) uniform writeonly image2D render_target;

layout(local_size_x=32, local_size_y=32, local_size_z=1) in;

layout(push_constant) uniform constants
{
   ivec3 dispatch_size;
} PushConstants;

uint pcg_hash(uint s)
{
    uint state = s * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

void main()
{
   uvec3 thread_id = gl_GlobalInvocationID;
   ivec2 image_size = imageSize(render_target);

   uint flatIndex = gl_GlobalInvocationID.z * (gl_NumWorkGroups.x * gl_WorkGroupSize.x) * (gl_NumWorkGroups.y * gl_WorkGroupSize.y) +
                 gl_GlobalInvocationID.y * (gl_NumWorkGroups.x * gl_WorkGroupSize.x) +
                 gl_GlobalInvocationID.x;

   uint hash_x = pcg_hash(flatIndex);

   vec4 out_color = vec4(float(thread_id.x) / float(PushConstants.dispatch_size.x), float(thread_id.y) / float(PushConstants.dispatch_size.y), 0, 1);
   
   imageStore(render_target, ivec2(thread_id.xy), out_color);
}