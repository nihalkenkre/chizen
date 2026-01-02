#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "utils.glsl"

layout(set=1, binding=2) buffer RandStates {
   uvec4 states[];
} rand_states;

layout(location = 0) rayPayloadInEXT vec3 hit_value;

void main()
{
    uint flat_index = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x;
    uvec4 z = rand_states.states[flat_index];

    hit_value.x = HybridTaus(z);
    hit_value.y = HybridTaus(z);
    hit_value.z = HybridTaus(z);

    rand_states.states[flat_index] = z;
}