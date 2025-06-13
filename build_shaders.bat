@echo off

cd %1
cd ..\..\shaders

:: delete local spv files
for /R %%F in (*.spv) do (
    echo delete %%F
    del /Q %%F
)

:: compile glsl in debug mode or release mode 
for /R %%F in (*.glsl) do (
    if "%3" == "Debug" (
       glslang %%F -gVS -Od --target-env vulkan1.3 -o %%F.spv
    )
    if "%3" == "Release" (
       glslang %%F --target-env vulkan1.3 -o %%F.spv
    )
)

cd %1
cd ..\..\cuda

:: delete local optix-ir files
for /R %%F in (*.optixir) do (
    echo delete %%F
    del /Q %%F
)

:: compile cu files in debug mode or release mode
for /R %%F in (*.cu) do (
    echo %%F
    if "%3" == "Debug" (
        nvcc --optix-ir -I"%OPTIX_PATH%"/include -I"%CUDA_PATH%"/include -I"%1../.." -m64 -G -rdc=true %%F -o %%F.optixir
    )

    if "%3" == "Release" (
        nvcc --optix-ir -I"%OPTIX_PATH%"/include -I"%CUDA_PATH%"/include -I"%1../.." -m64 -rdc=true --generate-line-info %%F -o %%F.optixir
    )
)

cd ..

:: delete compiled spv from target dir
echo deleting %2\shaders
if exist %2\shaders (
    rmdir /S /Q %2\shaders
)

:: delete compiled optixir from target dir
echo delete %2\cuda
if exist %2\cuda (
    rmdir /S /Q %2\cuda
)

:: copy optixir to target dir
xcopy /y /i /s %1..\..\cuda\*.optixir %2\cuda

:: copy spv to target dir
xcopy /y /i /s %1..\..\shaders\*.spv %2\shaders

:: copy hlsl to target dir
@REM xcopy /y /i /s %1..\..\shaders\*.hlsl %2\shaders