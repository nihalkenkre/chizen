@echo off

cd %1
cd ..\shaders

for /r %%F in (*.spv) do (
    echo delete %%F
    del /Q %%F
)

for /R %%F in (*.glsl) do (
    if "%3" == "Debug" (
       glslang %%F -gVS -Od --target-env vulkan1.3 -o %%F.spv
    )
    if "%3" == "Release" (
       glslang %%F --target-env vulkan1.3 -o %%F.spv
    )
)

cd ..

echo delete %2\shaders
if exist %2\shaders (
    rmdir /S /Q %2\shaders
)

xcopy /y /i /s %1..\shaders\*.spv %2\shaders
xcopy /y /i /s %1..\shaders\*.hlsl %2\shaders