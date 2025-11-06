: %1 Source dir
: %2 Build config name
: %3 Output dir

set source_dir=%1
set build_config_name=%2
set output_dir=%3

@echo off
echo deleting spv in %source_dir:/=\%\shaders

: delete local spv files
cd %source_dir:/=\%\shaders
for /R %%F in (*.spv) do (
   del /Q %%F
)

: building glsl
echo building glsl %source_dir:/=\%\shaders
for /R %%F in (*.glsl) do (
   if "%2" == "Debug" (
      glslang %%F -gVS -Od --target-env vulkan1.3 -o %%F.spv
   )
      
   if "%2" == "MinSizeRel" (
      glslang %%F --target-env vulkan1.3 -o %%F.spv
   )
)

: delete shaders folder from output_dir
echo delete %output_dir:/=\%\shaders
if exist %output_dir:/=\%\shaders (
   rmdir %output_dir:/=\%\shaders /s /q
)

: copy build spv to target dir
echo copying spv to %output_dir:/=\%\shaders
cd .. 
xcopy /y /i /s .\shaders\*.spv %output_dir:/=\%\shaders 