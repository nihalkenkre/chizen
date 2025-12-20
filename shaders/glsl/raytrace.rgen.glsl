#version 460
#extension GL_EXT_ray_tracing : enable

layout(set=0, binding=0, rgba32f) uniform image2D accum_target;
layout(set=0, binding=1, rgba32f) uniform writeonly image2D final_render;
layout(set=0, binding=2) buffer RandStates {
   uvec4 states[];
} rand_states;

layout(set=0, binding=3) uniform UniformBuffer {
   mat4 view_inverse;
   mat4 proj_inverse;
} uniform_buffer;

layout(set=0, binding=4) uniform accelerationStructureEXT tlas;

layout(push_constant) uniform PushConstants {
   uint curr_sample;
} pc;

layout(location=0) rayPayloadEXT vec3 hit_value;

uint TausStep(inout uint z, int S1, int S2, int S3, uint M)
{
  uint b = (((z << S1) ^ z) >> S2);
  return z = (((z & M) << S3) ^ b);
}

uint LCGStep(inout uint z, uint A, uint C)
{
  return z = (A * z + C);
}

float HybridTaus(inout uvec4 z)
{
   return 2.3283064365387e-10 * (
    TausStep(z.x, 13, 19, 12, 4294967294) ^
    TausStep(z.y, 2, 25, 4, 4294967288) ^  
    TausStep(z.z, 3, 11, 17, 4294967280) ^
    LCGStep(z.w, 1664525, 1013904223)
   );
}

void main()
{
   const vec2 pixel_center = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
   const vec2 in_uv = pixel_center / vec2(gl_LaunchSizeEXT.xy);
   vec2 d = in_uv * 2.f - 1.f;

   vec4 origin = uniform_buffer.view_inverse * vec4(0,0,0,1);
   vec4 target = uniform_buffer.proj_inverse * vec4(d.x, d.y, 1, 1);
   vec4 direction = uniform_buffer.view_inverse * vec4(normalize(target.xyz), 0);

   traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, origin.xyz, 0.001, direction.xyz, 1000.f, 0);

   imageStore(final_render, ivec2(gl_LaunchIDEXT.xy), vec4(hit_value, 1));
}