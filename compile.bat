@echo off

%VULKAN_SDK%/Bin/glslc.exe -fshader-stage=vertex shaders/vert.glsl -o shaders/vert.spv
%VULKAN_SDK%/Bin/glslc.exe -fshader-stage=fragment shaders/frag.glsl -o shaders/frag.spv
