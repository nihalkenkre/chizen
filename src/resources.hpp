#pragma once

#include <vma/vk_mem_alloc.h>

class ImageResource
{
public:
	ImageResource() = delete;
	ImageResource(const VkDevice device, const VkExtent3D& extent, const VkFormat format, const VkImageUsageFlags usage, const VmaAllocator allocator, const std::vector<uint32_t>& queue_family_indices, const std::string& name);

	ImageResource(const ImageResource& other) = delete;
	ImageResource& operator=(const ImageResource& other) = delete;

	~ImageResource() noexcept;

	//void ChangeImageLayout(const VkCommandBuffer cmd_buff,
	//	const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask,
	//	const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask,
	//	const VkImageLayout old_layout, const VkImageLayout new_layout,
	//	const uint32_t src_q_fly_idx, const uint32_t dst_q_fly_idx,
	//	const VkImageAspectFlags aspect_mask);

	VkImage GetImage() const;
	VkDescriptorImageInfo GetDescriptorInfo() const;

	VkExtent3D GetExtent() const;

	VmaAllocation GetAllocation() const;
	VmaAllocationInfo2 GetAllocationInfo2() const;

private:
	VkImage mImage = VK_NULL_HANDLE;
	VkDescriptorImageInfo mDescriptorInfo = {};
	VkExtent3D mExtent = {};

	VmaAllocation mAllocation = nullptr;
	VmaAllocationInfo2 mAllocationInfo = {};

	VmaAllocator mAllocator = nullptr;
	VkDevice mDevice = VK_NULL_HANDLE;
};

class BufferResource
{
public:
	BufferResource() = delete;
	BufferResource(const VkDevice device, const VmaAllocator allocator, const VkDeviceSize size, const VkBufferUsageFlags usage, const VmaAllocationCreateFlags vma_alloc_create_flags, const VmaMemoryUsage vma_mem_usage, const std::string& name);

	BufferResource(const BufferResource& other) = delete;
	BufferResource& operator=(const BufferResource& other) = delete;

	~BufferResource() noexcept;

	void CopyToBuffer(const VkCommandBuffer cmd_buff, const VkQueue queue, const VkBuffer dst_buffer, const VkDeviceSize size);

	VkDescriptorBufferInfo GetDescriptorInfo() const;

	VmaAllocation GetAllocation() const;
	VmaAllocationInfo2 GetAllocationInfo2() const;
	VkDeviceSize GetBufferSize() const;

	VkDeviceAddress GetDeviceAddress() const;
	VkDeviceOrHostAddressConstKHR GetDeviceOrHostAddressConstKHR() const;
	VkDeviceOrHostAddressKHR GetDeviceOrHostAddressKHR() const;

private:
	VkDescriptorBufferInfo mDescriptorInfo = {};

	VmaAllocation mAllocation = nullptr;
	VmaAllocationInfo2 mAllocationInfo = {};

	VkDeviceAddress mDeviceAddress = 0;
	VkDeviceOrHostAddressConstKHR mDeviceOrHostAddressConst = { 0 };
	VkDeviceOrHostAddressKHR mDeviceOrHostAddress = { 0 };
	VkDeviceSize mSize = 0;

	VmaAllocator mAllocator = nullptr;
	VkDevice mDevice = VK_NULL_HANDLE;
};

