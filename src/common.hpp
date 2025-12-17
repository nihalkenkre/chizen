#pragma once

extern "C" VkDevice g_device;
extern "C" VmaAllocator g_allocator;

extern "C" uint32_t g_graphics_queue_family_index;
extern "C" uint32_t g_compute_queue_family_index;
extern "C" uint32_t g_transfer_queue_family_index;