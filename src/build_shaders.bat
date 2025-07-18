@echo off

echo:
echo Building Optix Files
:: %1 Visual Studio Solution Dir
:: %2 Build Configuration
:: %3 Output Path

cd %1..\..\src\optix

:: delete local compiled optix-ir files
for /R %%F in (*.optixir) do (
    echo deleting %%F
    del /Q %%F
)

:: compile optix files in required configuration
for /R %%F in (*.cu) do (
    echo compiling %%F
    if "%2" == "Debug" (
        nvcc --optix-ir -I"%OPTIX_PATH%"/include -I"%OPTIX_PATH%"/SDK -I"%CUDA_PATH%"/include -I"%1../../" -Wno-deprecated-gpu-targets -m64 -G -rdc=true %%F -o %%F.optixir
    )

    if  "%2" == "Release" (
        nvcc --optix-ir -I"%OPTIX_PATH%"/include -I"%OPTIX_PATH%"/SDK -I"%CUDA_PATH%"/include -I"%1../../" -Wno-deprecated-gpu-targets -m64 -rdc=true %%F -o %%F.optixir
    )
)

:: cd to src folder
cd ..

:: delete compiled optixir from target dir
echo clearing %3optix
if exist %3\optix (
    rmdir /S /Q %3optix
)

::copy compiled optix-ir to target dir
echo Copying compiled optixir to %3optix
xcopy /y /i /s optix\*.optixir %3optix

:: delete local compiled optix-ir files
for /R %%F in (*.optixir) do (
    echo deleting %%F
    del /Q %%F
)