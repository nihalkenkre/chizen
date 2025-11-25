#include "resources.hpp"
#include "utils.hpp"

ImageResource ImageResource_Create(const VkDevice device, const VkExtent3D& extent, const VkFormat& format, const VkImageUsageFlags usage, const VmaAllocator allocator, const VmaAllocationCreateFlags vma_alloc_create_flags, const VmaMemoryUsage vma_mem_usage, const std::vector<uint32_t> queue_family_indices, const std::string& name)
{
	ImageResource ir = {
		.DescriptorInfo = {
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
		},
		.Extent = extent,
		.Allocator = allocator,
		.Device = device,
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
		.flags = vma_alloc_create_flags,
		.usage = vma_mem_usage,
	};

	VK_CHECK("create image", vmaCreateImage(allocator, &create_info, &alloc_ci, &ir.Image, &ir.Allocation, &ir.AllocationInfo.allocationInfo));

	const VkImageViewCreateInfo iv_ci = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = ir.Image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};

	VK_CHECK("create image view", vkCreateImageView(device, &iv_ci, nullptr, &ir.DescriptorInfo.imageView));

	const VkSamplerCreateInfo s_ci = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	};

	VK_CHECK("create sampler", vkCreateSampler(device, &s_ci, nullptr, &ir.DescriptorInfo.sampler));

	return ir;
}

void ImageResource_Destroy(ImageResource image_resource)
{
	if (image_resource.Device != VK_NULL_HANDLE)
	{
		vmaDestroyImage(image_resource.Allocator, image_resource.Image, image_resource.Allocation);
		vkDestroyImageView(image_resource.Device, image_resource.DescriptorInfo.imageView, nullptr);
		vkDestroySampler(image_resource.Device, image_resource.DescriptorInfo.sampler, nullptr);
	}
}

BufferResource BufferResource_Create(const VkDevice device, const VmaAllocator allocator, const VkDeviceSize size, const VkBufferUsageFlags usage, const VmaAllocationCreateFlags vma_alloc_create_flags, const VmaMemoryUsage vma_mem_usage, const std::string& name)
{
	BufferResource br = {
		.DescriptorInfo = {
			.range = VK_WHOLE_SIZE,
		},
		.Allocator = allocator,
		.Device = device,
	};

	const VkBufferCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
	};

	const VmaAllocationCreateInfo alloc_ci = {
		.flags = vma_alloc_create_flags,
		.usage = vma_mem_usage,
	};

	VK_CHECK("create buffer", vmaCreateBuffer(allocator, &create_info, &alloc_ci, &br.DescriptorInfo.buffer, &br.Allocation, &br.AllocationInfo.allocationInfo));

	return br;
}

void BufferResource_Destroy(BufferResource buffer_resource)
{
	if (buffer_resource.Device != VK_NULL_HANDLE)
		vmaDestroyBuffer(buffer_resource.Allocator, buffer_resource.DescriptorInfo.buffer, buffer_resource.Allocation);
}
