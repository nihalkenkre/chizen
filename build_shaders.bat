@echo off
SET current_path=%cd%

cd shaders

for /r %%F in (*.spv) do (
    del /Q %%F
)

for /R %%F in (*.glsl) do (
    glslang %%F --target-env vulkan1.3 -o %%F.spv
)

cd ..
