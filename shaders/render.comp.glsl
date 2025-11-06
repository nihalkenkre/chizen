#version 460

layout(set = 0, binding = 0, rgba32f) uniform image2D accum_target;
layout(set=0, binding=1, rgba32f) uniform writeonly image2D final_render;

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(push_constant) uniform PushConstants {
   ivec2 dispatch_size;
   uint current_time;
   uint curr_sample_is_reset;  // bit 17-1 curr_sample, bit 0 is_reset
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

void main() {
   uvec3 thread_id = gl_GlobalInvocationID;

   if (thread_id.x > pc.dispatch_size.x || thread_id.y > pc.dispatch_size.y)
      return;

   ivec2 image_size = imageSize(accum_target);

   uint flat_index = gl_GlobalInvocationID.z * (gl_NumWorkGroups.x * gl_WorkGroupSize.x) * (gl_NumWorkGroups.y * gl_WorkGroupSize.y) +
      gl_GlobalInvocationID.y * (gl_NumWorkGroups.x * gl_WorkGroupSize.x) +
      gl_GlobalInvocationID.x + pc.current_time;


   vec4 in_color = vec4(0, 0, 0, 1);

   uint curr_sample = (pc.curr_sample_is_reset & 0x1FFFE) >> 1;
   bool is_reset = (pc.curr_sample_is_reset & 1u) == 1;

   if (!is_reset) {
      in_color = imageLoad(accum_target, ivec2(thread_id.xy));
   }

   float hash_x = random(flat_index);
   float hash_y = random(flat_index);
   float hash_z = random(flat_index);

   vec4 out_color = vec4(hash_x, hash_y, hash_z, 1);

   imageStore(accum_target, ivec2(thread_id.xy), out_color + in_color);
   imageStore(final_render, ivec2(thread_id.xy), (out_color + in_color) / curr_sample);
}