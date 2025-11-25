#pragma once

#include "vulkan_interface.hpp"
#include "utils.hpp"

typedef struct Display Display;

Display* Display_Create(const VulkanInterface& vulkan_interface, const std::string& current_path);
void Display_Destroy(Display* d);

void Display_Render(Display* display, const float position_offset[], const float zoom_level, ImGUIState* imgui_state);
void Display_UpdateFinalRenderTarget(Display* display, const ImageResource FinalRenderTarget);