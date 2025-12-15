cmake_minimum_required(VERSION 3.10)
project(chizen LANGUAGES CXX)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Vulkan REQUIRED)

file(GLOB PROJECT_SRCS src/*)
list(FILTER PROJECT_SRCS EXCLUDE REGEX "imgui.ini")
file(GLOB IMGUI_SRCS imgui/*.cpp imgui/*.h imgui/backends/imgui_impl_sdl3.* imgui/backends/imgui_impl_vulkan.*)
file(GLOB IMGUI_FILE_DLG_SRCS ImGuiFileDialog/ImGuiFileDialog*.*)

source_group("ImGui" FILES ${IMGUI_SRCS})
source_group("ImGuiFileDialog" FILES ${IMGUI_FILE_DLG_SRCS})

add_compile_definitions(_HAS_EXCEPTIONS=0 SDL_MAIN_USE_CALLBACKS IMGUI_DEFINE_MATH_OPERATORS _CRT_SECURE_NO_WARNINGS GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_ENABLE_EXPERIMENTAL )

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
endif()

add_executable(${PROJECT_NAME} ${PROJECT_SRCS} ${IMGUI_SRCS} ${IMGUI_FILE_DLG_SRCS})

target_precompile_headers(${PROJECT_NAME} PRIVATE 
                                       <vector>
                                       <fstream>
                                       <chrono>
                                       <algorithm>
                                       <filesystem>
                                       <expected>
                                       <print>
                                       <SDL3/SDL.h>
	                                    <SDL3/SDL_main.h>
                                       <SDL3/SDL_vulkan.h>
                                       <imgui.h>
                                       <imgui_impl_sdl3.h>
                                       <imgui_impl_vulkan.h>
                                       <ImGuiFileDialog.h>
                                       <ImGuiFileDialogConfig.h>
                                       <slang/slang.h>
                                       <slang/slang-com-ptr.h>
                                       <vma/vk_mem_alloc.h>
                                       <glm/ext/matrix_clip_space.hpp>
                                       <glm/ext/matrix_transform.hpp>
                                       <glm/gtc/type_ptr.hpp>
                                       <cgltf.h>
                                       <tbb/parallel_for.h>
                                       <tbb/blocked_range2d.h>
                                       <tbb/task_arena.h>
                                       <embree4/rtcore.h>
                           )
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

target_include_directories(${PROJECT_NAME} PRIVATE SDL/include imgui imgui/backends ImGuiFileDialog glm cgltf stb $ENV{VULKAN_SDK}/Include $ENV{EMBREE_SDK}/include $ENV{ONE_TBB_SDK}/include)

target_link_directories(${PROJECT_NAME} PRIVATE $<IF:$<CONFIG:Debug>,${CMAKE_SOURCE_DIR}/SDL/build/Debug,> $<IF:$<CONFIG:MinSizeRel>,${CMAKE_SOURCE_DIR}/SDL/build/MinSizeRel,> $ENV{VULKAN_SDK}/Lib $ENV{EMBREE_SDK}/lib/ $ENV{ONE_TBB_SDK}/lib/intel64/vc14)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3-static Vulkan::Vulkan slang winmm setupapi version embree4 $<IF:$<CONFIG:Debug>,tbb12_debug,> $<IF:$<CONFIG:MinSizeRel>,tbb12,>)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_SOURCE_DIR}/build_shaders.bat ${CMAKE_SOURCE_DIR} $<CONFIG> $<TARGET_FILE_DIR:${PROJECT_NAME}>)
# TODO
# OTHER OS SHADER BUILD SCRIPT
endif()