@echo off

%VULKAN_SDK%/Bin/glslc.exe -fshader-stage=vertex shaders/scene_vert.glsl -o shaders/scene_vert.spv
%VULKAN_SDK%/Bin/glslc.exe -fshader-stage=fragment shaders/scene_frag.glsl -o shaders/scene_frag.spv

%VULKAN_SDK%/Bin/glslc.exe -fshader-stage=vertex shaders/gui_vert.glsl -o shaders/gui_vert.spv
%VULKAN_SDK%/Bin/glslc.exe -fshader-stage=fragment shaders/gui_frag.glsl -o shaders/gui_frag.spv

xcopy shaders\*.spv target\debug\shaders\ /y 
xcopy shaders\*.spv target\release\shaders\ /y