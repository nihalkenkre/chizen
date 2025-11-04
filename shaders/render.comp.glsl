#version 460

#define UINT32_MAX 0xFFFFFFFF

layout(set=0, binding=0, rgba32f) uniform writeonly image2D render_target;

layout(local_size_x=32, local_size_y=32, local_size_z=1) in;

layout(push_constant) uniform PushConstants
{
   ivec3 render_dims;
   uint current_time;
} pc;

uint lcg_xs_24(inout uint state) {
   uint result = state * 747796405u + 2891336453u;
   uint hashed_result = result ^ (result >> 14);
   state = hashed_result;
   return hashed_result >> 8;
}

float random(inout uint state) {
   uint result = lcg_xs_24(state);
   const float inv_max_int = 1.0 / 16777216.0;

   return float(result) * inv_max_int;
}

void main()
{
   uvec3 thread_id = gl_GlobalInvocationID;

   if (thread_id.x > pc.render_dims.x || thread_id.y > pc.render_dims.y)
      return;

   ivec2 image_size = imageSize(render_target);

   uint flatIndex = gl_GlobalInvocationID.z * (gl_NumWorkGroups.x * gl_WorkGroupSize.x) * (gl_NumWorkGroups.y * gl_WorkGroupSize.y) +
                 gl_GlobalInvocationID.y * (gl_NumWorkGroups.x * gl_WorkGroupSize.x) +
                 gl_GlobalInvocationID.x + pc.current_time;

   float hash_x = random(flatIndex);
   float hash_y = random(flatIndex);
   float hash_z = random(flatIndex);

   vec4 out_color = vec4(hash_x, hash_y, hash_z, 1);
   
   imageStore(render_target, ivec2(thread_id.xy), out_color);
}