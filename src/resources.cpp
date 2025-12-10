#include "resources.hpp"
#include "utils.hpp"

ImageResource::ImageResource(const VkDevice device, const VkExtent3D& extent, const VkFormat format, const VkImageUsageFlags usage, const VmaAllocator allocator, const std::vector<uint32_t>& queue_family_indices, const std::string& name)
{
	mDescriptorInfo = {
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	mExtent = extent;
	mAllocator = allocator;
	mDevice = device;

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
		.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
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

//void ImageResource::ChangeImageLayout(const VkCommandBuffer cmd_buff, const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask, const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask, const VkImageLayout old_layout, const VkImageLayout new_layout, const uint32_t src_q_fly_idx, const uint32_t dst_q_fly_idx, const VkImageAspectFlags aspect_mask)
//{
//	const VkImageMemoryBarrier2 img_mem_barr = {
//		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
//		.srcStageMask = src_stage_mask,
//		.srcAccessMask = src_access_mask,
//		.dstStageMask = dst_stage_mask,
//		.dstAccessMask = dst_access_mask,
//		.oldLayout = old_layout,
//		.newLayout = new_layout,
//		.srcQueueFamilyIndex = src_q_fly_idx,
//		.dstQueueFamilyIndex = dst_q_fly_idx,
//		.image = mImage,
//		.subresourceRange = {
//			.aspectMask = aspect_mask,
//			.levelCount = 1,
//			.layerCount = 1,
//		},
//	};
//
//	const VkDependencyInfo dep_info = {
//		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
//		.imageMemoryBarrierCount = 1,
//		.pImageMemoryBarriers = &img_mem_barr,
//	};
//
//	vkCmdPipelineBarrier2KHR(cmd_buff, &dep_info);
//}

VkImage ImageResource::GetImage() const
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

BufferResource::BufferResource(const VkDevice device, const VmaAllocator allocator, const VkDeviceSize size, const VkBufferUsageFlags usage, const VmaAllocationCreateFlags vma_alloc_create_flags, const VmaMemoryUsage vma_mem_usage, const std::string& name)
{
	mDescriptorInfo = {
		.range = VK_WHOLE_SIZE,
	};
	mAllocator = allocator;
	mDevice = device;
	mSize = size;

	const VkBufferCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
	};

	const VmaAllocationCreateInfo alloc_ci = {
		.flags = vma_alloc_create_flags,
		.usage = vma_mem_usage,
	};

	VK_CHECK("create buffer", vmaCreateBuffer(allocator, &create_info, &alloc_ci, &mDescriptorInfo.buffer, &mAllocation, &mAllocationInfo.allocationInfo));

 if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR)
	{
		const VkBufferDeviceAddressInfoKHR addr_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR,
			.buffer = mDescriptorInfo.buffer,
		};

		mDeviceAddress = vkGetBufferDeviceAddressKHR(mDevice, &addr_info);
		mDeviceOrHostAddress.deviceAddress = mDeviceAddress;
		mDeviceOrHostAddressConst.deviceAddress = mDeviceAddress;
	}

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(mDescriptorInfo.buffer), std::string(name).append(" buffer").c_str());
#endif // _DEBUG
}

BufferResource::~BufferResource() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
		vmaDestroyBuffer(mAllocator, mDescriptorInfo.buffer, mAllocation);
}

void BufferResource::CopyToBuffer(const VkCommandBuffer cmd_buff, const VkQueue queue, const VkBuffer dst_buffer, const VkDeviceSize size)
{
	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VK_CHECK("begin xfer cmd buff", vkBeginCommandBuffer(cmd_buff, &begin_info));

	const VkBufferCopy2 regions[] = {
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
			.size = size,
		},
	};

	const VkCopyBufferInfo2 copy_buff_info = {
		.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
		.srcBuffer = mDescriptorInfo.buffer,
		.dstBuffer = dst_buffer,
		.regionCount = std::size(regions),
		.pRegions = regions,
	};

	vkCmdCopyBuffer2KHR(cmd_buff, &copy_buff_info);
	VK_CHECK("end xfer cmd buff", vkEndCommandBuffer(cmd_buff));

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = cmd_buff,
		},
	};

	const VkSubmitInfo2 submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = std::size(cmd_buff_infos),
			.pCommandBufferInfos = cmd_buff_infos,
		},
	};

	VK_CHECK("submit geom buffer xfer cmd", vkQueueSubmit2KHR(queue, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));
	VK_CHECK("wait xfer cmd buff", vkQueueWaitIdle(queue));
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

VkDeviceSize BufferResource::GetBufferSize() const
{
	return mSize;
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
