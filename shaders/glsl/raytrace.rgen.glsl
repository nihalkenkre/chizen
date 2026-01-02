#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "utils.glsl"

layout(set=0, binding=0) uniform CameraInfo {
   mat4 view_inverse;
   mat4 proj_inverse;
} camera_info;

layout(set=1, binding=0, rgba32f) uniform image2D accum_target;
layout(set=1, binding=1, rgba32f) uniform writeonly image2D final_render;
layout(set=1, binding=2) buffer RandStates {
   uvec4 states[];
} rand_states;

layout(set=1, binding=3) uniform accelerationStructureEXT tlas;

layout(push_constant) uniform PushConstants {
   uint curr_sample;
} pc;

layout(location=0) rayPayloadEXT vec3 hit_value;

void main()
{
   const vec2 pixel_center = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
   const vec2 in_uv = pixel_center / vec2(gl_LaunchSizeEXT.xy);
   vec2 d = in_uv * 2.f - 1.f;

   vec4 origin = camera_info.view_inverse * vec4(0,0,0,1);
   vec4 target = camera_info.proj_inverse * vec4(d.x, d.y, 1, 1);
   vec4 direction = camera_info.view_inverse * vec4(normalize(target.xyz), 0);

   traceRayEXT(tlas, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, origin.xyz, 0.001, direction.xyz, 1000.f, 0);
   //  uint flat_index = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x;
   //  uvec4 z = rand_states.states[flat_index];

   //  hit_value.x = HybridTaus(z);
   //  hit_value.y = HybridTaus(z);
   //  hit_value.z = HybridTaus(z);

   imageStore(final_render, ivec2(gl_LaunchIDEXT.xy), vec4(hit_value, 1));
   //  rand_states.states[flat_index] = z;
}