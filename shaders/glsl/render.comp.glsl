#version 460

layout(set=0, binding=0, rgba32f) uniform image2D accum_target;
layout(set=0, binding=1, rgba32f) uniform writeonly image2D final_render;
layout(set=0, binding=2) buffer RandStates {
   uint states[];
} rand_states;

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(push_constant) uniform PushConstants {
   ivec2 dispatch_size;
   uint curr_sample;
} pc;

uint TausStep(inout uint z, int S1, int S2, int S3, uint M)
{
  uint b = (((z << S1) ^ z) >> S2);
  return z = (((z & M) << S3) ^ b);
}

uint LCGStep(inout uint z, uint A, uint C)
{
  return z = (A * z + C);
}

float HybridTaus(inout uint z1, inout uint z2, inout uint z3, inout uint z4)
{
   return 2.3283064365387e-10 * (
    TausStep(z1, 13, 19, 12, 4294967294) ^
    TausStep(z2, 2, 25, 4, 4294967288) ^  
    TausStep(z3, 3, 11, 17, 4294967280) ^
    LCGStep(z4, 1664525, 1013904223)
   );
}

void main() {
   uvec3 thread_id = gl_GlobalInvocationID;

   if (thread_id.x > pc.dispatch_size.x || thread_id.y > pc.dispatch_size.y)
      return;

   uint flat_index = gl_GlobalInvocationID.z * (gl_NumWorkGroups.x * gl_WorkGroupSize.x) * (gl_NumWorkGroups.y * gl_WorkGroupSize.y) +
      gl_GlobalInvocationID.y * (gl_NumWorkGroups.x * gl_WorkGroupSize.x) +
      gl_GlobalInvocationID.x;

   uint z1 = rand_states.states[flat_index];
   uint z2 = rand_states.states[flat_index + 1];
   uint z3 = rand_states.states[flat_index + 2];
   uint z4 = rand_states.states[flat_index + 3];

   float hash_x = HybridTaus(z1, z2, z3, z4);
   float hash_y = HybridTaus(z1, z2, z3, z4);
   float hash_z = HybridTaus(z1, z2, z3, z4);

   vec4 out_color = vec4(hash_x, hash_y, hash_z, 1);

   vec4 in_color = imageLoad(accum_target, ivec2(thread_id.xy));
   imageStore(accum_target, ivec2(thread_id.xy), out_color + in_color);
   imageStore(final_render, ivec2(thread_id.xy), (out_color + in_color) / pc.curr_sample);

   rand_states.states[flat_index] = z1;
   rand_states.states[flat_index + 1] = z2;
   rand_states.states[flat_index + 2] = z3;
   rand_states.states[flat_index + 3] = z4;
}