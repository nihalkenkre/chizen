#pragma once

#include <vma/vk_mem_alloc.h>

struct ImageResource
{
	VkImage Image = VK_NULL_HANDLE;
	VkDescriptorImageInfo DescriptorInfo = {};
	VkExtent3D Extent = {};

	VmaAllocation Allocation = nullptr;
	VmaAllocationInfo2 AllocationInfo = {};

	VmaAllocator Allocator = nullptr;
	VkDevice Device = VK_NULL_HANDLE;
};

ImageResource ImageResource_Create(const VkDevice device, const VkExtent3D& extent, const VkFormat& format, const VkImageUsageFlags usage, const VmaAllocator allocator, const VmaAllocationCreateFlags vma_alloc_create_flags, const VmaMemoryUsage vma_mem_usage, const std::vector<uint32_t> queue_family_indices, const std::string& name);
void ImageResource_Destroy(ImageResource image_resource);

struct BufferResource
{
	VkDescriptorBufferInfo DescriptorInfo = {};

	VmaAllocation Allocation = nullptr;
	VmaAllocationInfo2 AllocationInfo = {};

	VmaAllocator Allocator = nullptr;
	VkDevice Device = VK_NULL_HANDLE;
};

BufferResource BufferResource_Create(const VkDevice device, const VmaAllocator allocator, const VkDeviceSize size, const VkBufferUsageFlags usage, const VmaAllocationCreateFlags vma_alloc_create_flags, const VmaMemoryUsage vma_mem_usage, const std::string& name);
void BufferResource_Destroy(BufferResource buffer_resource);
