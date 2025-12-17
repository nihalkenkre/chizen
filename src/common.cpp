#include "common.hpp"

VkDevice g_device = VK_NULL_HANDLE;
VmaAllocator g_allocator = VK_NULL_HANDLE;

uint32_t g_graphics_queue_family_index = 0;
uint32_t g_compute_queue_family_index = 0;
uint32_t g_transfer_queue_family_index = 0;