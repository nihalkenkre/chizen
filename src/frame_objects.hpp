#pragma once

#include "vulkan_interface.hpp"

typedef struct FrameObjects FrameObjects;

FrameObjects* FrameObjects_Create(const VkDevice device, const uint32_t queue_family_index, const uint8_t& max_frames_in_flight);
void FrameObjects_Destroy(FrameObjects* fo);

uint8_t FrameObjects_GetFrameInFlight(FrameObjects* fo);
VkCommandBuffer FrameObjects_GetCommandBuffer(FrameObjects* fo);
VkSemaphore FrameObjects_GetSemaphore(FrameObjects* fo);
uint64_t& FrameObjects_GetFrameSemValue(FrameObjects* fo);
void FrameObjects_NextFrame(FrameObjects* fo);