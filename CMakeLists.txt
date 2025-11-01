cmake_minimum_required(VERSION 3.10)
project(chizen LANGUAGES CXX)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# add_subdirectory(SDL)
find_package(Vulkan REQUIRED)

file(GLOB PROJECT_SRCS src/*)
file(GLOB SHADER_SRCS shaders/*)
file(GLOB IMGUI_SRCS imgui/*.cpp imgui/*.h imgui/backends/imgui_impl_sdl3.* imgui/backends/imgui_impl_vulkan.*)
file(GLOB IMGUI_FILE_DLG_SRCS ImGuiFileDialog/ImGuiFileDialog*.*)

source_group("Shaders" FILES ${SHADER_SRCS})
source_group("ImGui" FILES ${IMGUI_SRCS})
source_group("ImGuiFileDialog" FILES ${IMGUI_FILE_DLG_SRCS})

add_compile_definitions(_HAS_EXCEPTIONS=0 SDL_MAIN_USE_CALLBACKS IMGUI_DEFINE_MATH_OPERATORS)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
endif()

add_executable(${PROJECT_NAME} ${PROJECT_SRCS} ${SHADER_SRCS} ${IMGUI_SRCS} ${IMGUI_FILE_DLG_SRCS})

target_precompile_headers(${PROJECT_NAME} PRIVATE 
                                       <vector>
                                       <fstream>
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
                           )
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

target_include_directories(${PROJECT_NAME} PRIVATE SDL/include imgui imgui/backends ImGuiFileDialog $ENV{VULKAN_SDK}/Include)

target_link_directories(${PROJECT_NAME} PRIVATE $<IF:$<CONFIG:Debug>,${CMAKE_SOURCE_DIR}/SDL/build/Debug,> $<IF:$<CONFIG:MinSizeRel>,${CMAKE_SOURCE_DIR}/SDL/build/MinSizeRel,> $ENV{VULKAN_SDK}/Lib)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3-static Vulkan::Vulkan slang winmm setupapi version)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_SOURCE_DIR}/build_shaders.bat ${CMAKE_SOURCE_DIR} $<CONFIG> $<TARGET_FILE_DIR:${PROJECT_NAME}>)
# TODO
# NON WINDOWS SHADER BUILD SCRIPT
endif()