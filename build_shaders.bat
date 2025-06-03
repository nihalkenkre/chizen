@echo off

cd %1
cd ..\shaders

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

@REM cd %1
@REM cd ..\cuda

@REM :: delete local optix-ir files
@REM for /R %%F in (*.optixir) do (
@REM     echo delete %%F
@REM     del /Q %%F
@REM )

@REM :: compile cu files in debug mode or release mode
@REM for /R %%F in (*.cu) do (
@REM     echo %%F
@REM     if "%3" == "Debug" (
@REM         nvcc --optix-ir -I"%OPTIX_PATH%"/include -I"%CUDA_PATH%"/include -m64 -G -rdc=true %%F -o %%F.optixir
@REM     )

@REM     if "%3" == "Release" (
@REM         nvcc --optix-ir -I"%OPTIX_PATH%"/include -I"%CUDA_PATH%"/include -m64 -rdc=true --use_fast_math --generate-line-info %%F -o %%F.optixir
@REM     )
@REM )

cd ..

:: delete compiled spv from target dir
echo delete %2\shaders
if exist %2\shaders (
    rmdir /S /Q %2\shaders
)

:: delete compiled optixir from target dir
@REM echo delete %2\cuda
@REM if exist %2\cuda (
@REM     rmdir /S /Q %2\cuda
@REM )

:: copy optixir to target dir
@REM xcopy /y /i /s %1..\cuda\*.optixir %2\cuda

:: copy spv to target dir
xcopy /y /i /s %1..\shaders\*.spv %2\shaders

:: copy hlsl to target dir
xcopy /y /i /s %1..\shaders\*.hlsl %2\shaders