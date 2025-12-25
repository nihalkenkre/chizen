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

	VkImage GetVkImage() const;
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
	VkBuffer GetVkBuffer() const;
	VkDescriptorBufferInfo GetDescriptorInfo() const;

	VmaAllocation GetAllocation() const;
	VmaAllocationInfo2 GetAllocationInfo2() const;

	VkDeviceAddress GetDeviceAddress() const;
	VkDeviceOrHostAddressConstKHR GetDeviceOrHostAddressConstKHR() const;
	VkDeviceOrHostAddressKHR GetDeviceOrHostAddressKHR() const;

protected:
	BufferResource() {}

	BufferResource(const BufferResource& other) = delete;
	BufferResource& operator=(const BufferResource& other) = delete;

	virtual ~BufferResource() noexcept {};

	VkDescriptorBufferInfo mDescriptorInfo = {};

	VmaAllocation mAllocation = nullptr;
	VmaAllocationInfo2 mAllocationInfo = {};

	VkDeviceAddress mDeviceAddress = 0;
	VkDeviceOrHostAddressConstKHR mDeviceOrHostAddressConst = { 0 };
	VkDeviceOrHostAddressKHR mDeviceOrHostAddress = { 0 };

	VmaAllocator mAllocator = nullptr;
	VkDevice mDevice = VK_NULL_HANDLE;
};

class HostBufferResource : public BufferResource
{
public:
	HostBufferResource() = delete;
	HostBufferResource(const VkDevice device, const VmaAllocator allocator, const VkBufferUsageFlags usage, const VmaAllocationCreateFlags vma_alloc_create_flags, const std::vector<uint8_t>& data, const std::string& name, const std::vector<uint32_t>& queue_family_indices = {});
	HostBufferResource(const VkDevice device, const VmaAllocator allocator, const VkBufferUsageFlags usage, const VmaAllocationCreateFlags vma_alloc_create_flags, const VkDeviceSize size, const std::string& name, const std::vector<uint32_t>& queue_family_indices = {});

	~HostBufferResource() noexcept;
};

class DeviceBufferResource : public BufferResource
{
public:
	DeviceBufferResource() = delete;
	DeviceBufferResource(const VkDevice device, const VmaAllocator allocator, const VkBufferUsageFlags usage, const VkDeviceSize size, const std::string& name);

	~DeviceBufferResource() noexcept;
};
