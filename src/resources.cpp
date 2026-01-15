#include "resources.hpp"
#include "utils.hpp"

ImageResource::ImageResource(const VkDevice device, const VkExtent3D& extent, const VkFormat format, const VkImageUsageFlags usage, const VmaAllocator allocator, const std::vector<uint32_t>& queue_family_indices, const std::string& name)
	: mExtent(extent), mAllocator(allocator), mDevice(device)
{
	mDescriptorInfo = {
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};

	const VkImageCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = extent,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_CONCURRENT,
		.queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size()),
		.pQueueFamilyIndices = queue_family_indices.data(),
	};

	const VmaAllocationCreateInfo alloc_ci = {
		.usage = VMA_MEMORY_USAGE_AUTO,
	};

	VK_CHECK("create image", vmaCreateImage(allocator, &create_info, &alloc_ci, &mImage, &mAllocation, &mAllocationInfo.allocationInfo));

	VkImageViewCreateInfo iv_ci = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = mImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};

	if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
		format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D16_UNORM_S8_UINT ||
		format == VK_FORMAT_D16_UNORM || format == VK_FORMAT_D16_UNORM_S8_UINT)
	{
		iv_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	VK_CHECK("create image view", vkCreateImageView(device, &iv_ci, nullptr, &mDescriptorInfo.imageView));

	const VkSamplerCreateInfo s_ci = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	};

	VK_CHECK("create sampler", vkCreateSampler(device, &s_ci, nullptr, &mDescriptorInfo.sampler));

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(mImage), std::string(name).append(" image").c_str());
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64_t>(mDescriptorInfo.imageView), std::string(name).append(" image view").c_str());
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<uint64_t>(mDescriptorInfo.sampler), std::string(name).append(" sampler").c_str());
#endif	// _DEBUG
}

ImageResource::~ImageResource() noexcept
{
	if (mDevice != VK_NULL_HANDLE && mAllocator != nullptr)
	{
		vmaDestroyImage(mAllocator, mImage, mAllocation);
		vkDestroyImageView(mDevice, mDescriptorInfo.imageView, nullptr);
		vkDestroySampler(mDevice, mDescriptorInfo.sampler, nullptr);
	}
}

VkImage ImageResource::GetVkImage() const
{
	return mImage;
}

VkDescriptorImageInfo ImageResource::GetDescriptorInfo() const
{
	return mDescriptorInfo;
}

VkExtent3D ImageResource::GetExtent() const
{
	return mExtent;
}

VmaAllocation ImageResource::GetAllocation() const
{
	return mAllocation;
}

VmaAllocationInfo2 ImageResource::GetAllocationInfo2() const
{
	return mAllocationInfo;
}

VkBuffer BufferResource::GetVkBuffer() const
{
	return mDescriptorInfo.buffer;
}

VkDescriptorBufferInfo BufferResource::GetDescriptorInfo() const
{
	return mDescriptorInfo;
}

VmaAllocation BufferResource::GetAllocation() const
{
	return mAllocation;
}

VmaAllocationInfo2 BufferResource::GetAllocationInfo2() const
{
	return mAllocationInfo;
}

VkDeviceAddress BufferResource::GetDeviceAddress() const
{
	return mDeviceAddress;
}

VkDeviceOrHostAddressConstKHR BufferResource::GetDeviceOrHostAddressConstKHR() const
{
	return mDeviceOrHostAddressConst;
}

VkDeviceOrHostAddressKHR BufferResource::GetDeviceOrHostAddressKHR() const
{
	return mDeviceOrHostAddress;
}

HostBufferResource::HostBufferResource(const VkDevice device, const VmaAllocator allocator, const VkBufferUsageFlags usage, const VmaAllocationCreateFlags vma_alloc_create_flags, const std::vector<uint8_t>& data, const std::string& name, const std::vector<uint32_t>& queue_family_indices)
{
	mDevice = device;
	mAllocator = allocator;

	mDescriptorInfo = {
		.range = VK_WHOLE_SIZE,
	};

	const VkBufferCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = data.size(),
		.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		.sharingMode = static_cast<VkSharingMode>(std::clamp(static_cast<int>(queue_family_indices.size()), 0, 1)),
		.queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size()),
		.pQueueFamilyIndices = queue_family_indices.data(),
	};

	const VmaAllocationCreateInfo alloc_ci = {
		.flags = vma_alloc_create_flags | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO,
	};

	VK_CHECK("create buffer", vmaCreateBuffer(allocator, &create_info, &alloc_ci, &mDescriptorInfo.buffer, &mAllocation, &mAllocationInfo.allocationInfo));

	const VkBufferDeviceAddressInfoKHR addr_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
		.buffer = mDescriptorInfo.buffer,
	};

	mDeviceAddress = vkGetBufferDeviceAddress(mDevice, &addr_info);
	mDeviceOrHostAddress.deviceAddress = mDeviceAddress;
	mDeviceOrHostAddressConst.deviceAddress = mDeviceAddress;

	std::memcpy(mAllocationInfo.allocationInfo.pMappedData, data.data(), data.size());

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(mDescriptorInfo.buffer), std::string(name).append(" buffer").c_str());
#endif // _DEBUG
}

HostBufferResource::HostBufferResource(const VkDevice device, const VmaAllocator allocator, const VkBufferUsageFlags usage, const VmaAllocationCreateFlags vma_alloc_create_flags, const VkDeviceSize size, const std::string& name, const std::vector<uint32_t>& queue_family_indices)
{
	mDevice = device;
	mAllocator = allocator;

	mDescriptorInfo = {
		.range = VK_WHOLE_SIZE,
	};

	const VkBufferCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		.sharingMode = static_cast<VkSharingMode>(std::clamp(static_cast<int>(queue_family_indices.size()), 0, 1)),
		.queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size()),
		.pQueueFamilyIndices = queue_family_indices.data(),
	};

	const VmaAllocationCreateInfo alloc_ci = {
		.flags = vma_alloc_create_flags | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO,
	};

	VK_CHECK("create buffer", vmaCreateBuffer(allocator, &create_info, &alloc_ci, &mDescriptorInfo.buffer, &mAllocation, &mAllocationInfo.allocationInfo));

	const VkBufferDeviceAddressInfoKHR addr_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
		.buffer = mDescriptorInfo.buffer,
	};

	mDeviceAddress = vkGetBufferDeviceAddress(mDevice, &addr_info);
	mDeviceOrHostAddress.deviceAddress = mDeviceAddress;
	mDeviceOrHostAddressConst.deviceAddress = mDeviceAddress;

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(mDescriptorInfo.buffer), std::string(name).append(" buffer").c_str());
#endif // _DEBUG
}

HostBufferResource::~HostBufferResource() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
		vmaDestroyBuffer(mAllocator, mDescriptorInfo.buffer, mAllocation);
}

DeviceBufferResource::DeviceBufferResource(const VkDevice device, const VmaAllocator allocator, const VkBufferUsageFlags usage, const VkDeviceSize size, const std::string& name, const VmaPool mem_pool)
{
	mDevice = device;
	mAllocator = allocator;

	mDescriptorInfo = {
		.range = VK_WHOLE_SIZE,
	};

	const VkBufferCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	};

	const VmaAllocationCreateInfo alloc_ci = {
		.usage = VMA_MEMORY_USAGE_AUTO,
		.pool = mem_pool
	};

	VK_CHECK("create buffer", vmaCreateBuffer(allocator, &create_info, &alloc_ci, &mDescriptorInfo.buffer, &mAllocation, &mAllocationInfo.allocationInfo));

	const VkBufferDeviceAddressInfoKHR addr_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
		.buffer = mDescriptorInfo.buffer,
	};

	mDeviceAddress = vkGetBufferDeviceAddress(mDevice, &addr_info);
	mDeviceOrHostAddress.deviceAddress = mDeviceAddress;
	mDeviceOrHostAddressConst.deviceAddress = mDeviceAddress;

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(mDescriptorInfo.buffer), std::string(name).append(" buffer").c_str());
#endif // _DEBUG
}

DeviceBufferResource::~DeviceBufferResource() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
		vmaDestroyBuffer(mAllocator, mDescriptorInfo.buffer, mAllocation);
}
