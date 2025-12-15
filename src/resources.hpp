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
	BufferResource(const VkDevice device, const VmaAllocator allocator, const std::vector<uint8_t>& data, const VkBufferUsageFlags usage, const VmaAllocationCreateFlags vma_alloc_create_flags, const std::string& name, const VkCommandBuffer cmd_buff = VK_NULL_HANDLE, const VkQueue queue = VK_NULL_HANDLE);
	BufferResource(const VkDevice device, const VmaAllocator allocator, const VkDeviceSize size, const VkBufferUsageFlags usage, const VmaAllocationCreateFlags vma_alloc_create_flags, const std::string& name, const VkCommandBuffer cmd_buff = VK_NULL_HANDLE, const VkQueue queue = VK_NULL_HANDLE);

	BufferResource(const BufferResource& other) = delete;
	BufferResource& operator=(const BufferResource& other) = delete;

	~BufferResource() noexcept;

	void CopyToBuffer(const VkCommandBuffer cmd_buff, const VkQueue queue, const VkBuffer dst_buffer, const VkDeviceSize size);
	void CopyToImage(const VkCommandBuffer cmd_buff, const VkQueue queue, const VkImage dst_image, const VkExtent2D dst_image_extent);

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

