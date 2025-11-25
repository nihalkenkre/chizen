#pragma once

#include "vulkan_interface.hpp"

typedef struct Raytrace Raytrace;

Raytrace* Raytrace_Create(const VulkanInterface* const vulkan_interface, const VkExtent2D& extent, const std::string& current_path);
void Raytrace_Destroy(Raytrace* r);

void Raytrace_RecreateRenderResources(Raytrace* r, const VkExtent2D& extent);
void Raytrace_Render(Raytrace* r, bool* is_rendering, const uint32_t max_samples);
void Raytrace_UpdateFinalRenderTarget(Raytrace* r, const ImageResource FinalRenderTarget);
void Raytrace_StopRendering(Raytrace* r);