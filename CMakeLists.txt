cmake_minimum_required(VERSION 3.10)
project(chizen LANGUAGES CXX CUDA)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(SDL)
find_package(CUDAToolkit REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB PROJECT_SRCS src/*)
file(GLOB IMGUI_SRCS imgui/*.cpp imgui/*.h imgui/backends/imgui_impl_sdlrenderer3.* imgui/backends/imgui_impl_sdl3.* imgui/backends/imgui_impl_vulkan.*)
file(GLOB IMGUI_FILE_DLG_SRCS ImGuiFileDialog/ImGuiFileDialog*.*)

add_executable(${PROJECT_NAME} ${PROJECT_SRCS} ${IMGUI_SRCS} ${IMGUI_FILE_DLG_SRCS})

add_compile_definitions(_HAS_EXCEPTIONS=0 SDL_MAIN_USE_CALLBACKS IMGUI_DEFINE_MATH_OPERATORS)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
endif()

target_precompile_headers(${PROJECT_NAME} PRIVATE 
                                       <vector>
                                       <fstream>
                                       <filesystem>
                                       <expected>
                                       <print>
                                       <SDL3/SDL.h>
	                                    <SDL3/SDL_main.h>
                                        <SDL3/SDL_vulkan.h>
                                        <imgui.h>
                                        <imgui_impl_sdl3.h>
                                        <imgui_impl_sdlrenderer3.h>
                                        <imgui_impl_vulkan.h>
                                        <ImGuiFileDialog.h>
                                        <ImGuiFileDialogConfig.h>
                                        <thread>
                                        <algorithm>
                           )
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
set_property(TARGET ${PROJECT_NAME} PROPERTY CUDA_SEPARABLE_COMPILATION ON)
set_property(TARGET ${PROJECT_NAME} PROPERTY CUDA_ARCHITECTURES native)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Wno-deprecated-gpu-targets")

target_include_directories(${PROJECT_NAME} PRIVATE SDL/include imgui imgui/backends ImGuiFileDialog $ENV{VULKAN_SDK}/Include)
target_link_directories(${PROJECT_NAME} PRIVATE SDL)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3-static CUDA::cudart_static Vulkan::Vulkan)