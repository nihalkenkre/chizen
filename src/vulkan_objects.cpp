#include "vulkan_objects.hpp"
#include "utils.hpp"
#include "vulkan_functions.hpp"
#include "vulkan_interface.hpp"

Instance::Instance(const char* const* extensions, const uint32_t extensions_count)
{
	std::vector<const char*> req_ext_names;

	for (uint32_t ext_idx = 0; ext_idx < extensions_count; ++ext_idx)
	{
		req_ext_names.push_back(extensions[ext_idx]);
	}

	std::vector<const char*> more_exts = {
		VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME,
		VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
#ifdef _DEBUG
		  VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
	};
	req_ext_names.append_range(more_exts);

	uint32_t property_count = 0;
	VK_CHECK("enumerate instance extensions", vkEnumerateInstanceExtensionProperties(nullptr, &property_count, nullptr));

	std::vector<VkExtensionProperties> props(property_count);
	VK_CHECK("enumerate instance extensions", vkEnumerateInstanceExtensionProperties(nullptr, &property_count, props.data()));

	for (auto const& req_ext_name : req_ext_names)
	{
		auto it = std::find_if(props.begin(), props.end(), [&](const VkExtensionProperties prop)
			{ return (std::strcmp(prop.extensionName, req_ext_name) == 0); });

		if (it == props.end())
		{
			std::println("Extension {} is not supporeted by instance", req_ext_name);
		}
	}

	const VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Chizen",
		.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
		.pEngineName = "Chizen",
		.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
		.apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0),
	};

	const VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = static_cast<uint32_t>(req_ext_names.size()),
		.ppEnabledExtensionNames = req_ext_names.data(),
	};

	VK_CHECK("create instance", vkCreateInstance(&create_info, nullptr, &mInstance));

	vk_SetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(mInstance, "vkSetDebugUtilsObjectNameEXT"));
}

Instance::~Instance() noexcept
{
	vkDestroyInstance(mInstance, nullptr);
}

VkInstance Instance::GetInstance() const
{
	return mInstance;
}

PhysicalDeviceData Instance::GetPhysicalDeviceData(const VkSurfaceKHR& surface) const
{
	uint32_t physical_device_count = 0;
	VK_CHECK("enumerate physical devices", vkEnumeratePhysicalDevices(mInstance, &physical_device_count, nullptr));

	std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
	VK_CHECK("enumerate physical devices", vkEnumeratePhysicalDevices(mInstance, &physical_device_count, physical_devices.data()));

	PhysicalDeviceData pdd = {
		.Properties = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
			.pNext = &pdd.RayTracingProperties,
		},
		.MemoryProperties = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
		},
		.RayTracingProperties = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
		}
	};

	for (auto const& PhysicalDevice : physical_devices)
	{
		vkGetPhysicalDeviceProperties2(PhysicalDevice, &pdd.Properties);

		if (pdd.Properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			uint32_t q_fly_cnt = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &q_fly_cnt, nullptr);

			std::vector<VkQueueFamilyProperties> q_fly_props(q_fly_cnt);
			vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &q_fly_cnt, q_fly_props.data());

			// find graphics queue
			for (uint32_t q = 0; q < q_fly_cnt; ++q)
			{
				VkBool32 is_supported = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, q, surface, &is_supported);

				if (is_supported &&
					SDL_Vulkan_GetPresentationSupport(mInstance, PhysicalDevice, q) &&
					q_fly_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					pdd.PhysicalDevice = PhysicalDevice;
					pdd.GraphicsQueueFamilyIndex = q;

					vkGetPhysicalDeviceMemoryProperties2(PhysicalDevice, &pdd.MemoryProperties);

					break;
				}
			}

			// Find transfer queue
			for (int32_t q = q_fly_cnt - 1; q >= 0; --q)
			{
				if ((q_fly_props[q].queueFlags & VK_QUEUE_TRANSFER_BIT) && !(q_fly_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(q_fly_props[q].queueFlags & VK_QUEUE_COMPUTE_BIT))
				{
					pdd.TransferQueueFamilyIndex = q;
					break;
				}
				else if ((q_fly_props[q].queueFlags & VK_QUEUE_TRANSFER_BIT) && !(q_fly_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT))
				{
					pdd.TransferQueueFamilyIndex = q;
					break;
				}
				else if (q_fly_props[q].queueFlags & VK_QUEUE_TRANSFER_BIT)
				{
					pdd.TransferQueueFamilyIndex = q;
					break;
				}
			}

			// Find compute queue
			for (int32_t q = q_fly_cnt - 1; q >= 0; --q)
			{
				if ((q_fly_props[q].queueFlags & VK_QUEUE_COMPUTE_BIT) && !(q_fly_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(q_fly_props[q].queueFlags & VK_QUEUE_TRANSFER_BIT))
				{
					pdd.ComputeQueueFamilyIndex = q;
					break;
				}
				else if ((q_fly_props[q].queueFlags & VK_QUEUE_COMPUTE_BIT) && !(q_fly_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT))
				{
					pdd.ComputeQueueFamilyIndex = q;
					break;
				}
				else if (q_fly_props[q].queueFlags & VK_QUEUE_COMPUTE_BIT)
				{
					pdd.ComputeQueueFamilyIndex = q;
					break;
				}
			}
		}
	}

	return pdd;
}

Surface::Surface(SDL_Window* window, const VkInstance& instance)
{
	mInstance = instance;

	SDL_Vulkan_CreateSurface(window, instance, nullptr, &mSurface);
}

Surface::~Surface() noexcept
{
	if (mInstance != VK_NULL_HANDLE)
		vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
}

VkSurfaceKHR Surface::GetSurfaceKHR() const
{
	return mSurface;
}

VkPresentModeKHR Surface::GetPresentMode() const
{
	return mPresentMode;
}

VkSurfaceFormatKHR Surface::GetSurfaceFormat() const
{
	return mSurfaceFormat;
}

VkSurfaceCapabilities2KHR Surface::GetSurfaceCapabilities() const
{
	return mSurfaceCapabilities;
}

void Surface::PopulateSurfaceData(const VkPhysicalDevice& physical_device)
{
	{
		uint32_t surf_forms_count = 0;
		VK_CHECK("get surface formats", vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, mSurface, &surf_forms_count, nullptr));

		std::vector<VkSurfaceFormatKHR> surf_forms(surf_forms_count);
		VK_CHECK("get surface formats", vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, mSurface, &surf_forms_count, surf_forms.data()));

		auto it = std::find_if(surf_forms.begin(), surf_forms.end(), [](const VkSurfaceFormatKHR frm)
			{ return (frm.format == VK_FORMAT_R8G8B8A8_UNORM) && (frm.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR); });

		if (it != surf_forms.end())
		{
			mSurfaceFormat = *it;
		}
	}

	{
		uint32_t present_modes_count = 0;
		VK_CHECK("get present modes", vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, mSurface, &present_modes_count, nullptr));

		std::vector<VkPresentModeKHR> present_modes(present_modes_count);
		VK_CHECK("get surface formats", vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, mSurface, &present_modes_count, present_modes.data()));

		auto it = std::find_if(present_modes.begin(), present_modes.end(), [](const VkPresentModeKHR mode)
			{ return mode == VK_PRESENT_MODE_MAILBOX_KHR; });

		if (it != present_modes.end())
		{
			mPresentMode = *it;
		}
	}

	const VkPhysicalDeviceSurfaceInfo2KHR surface_info = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
		.surface = mSurface,
	};

	VK_CHECK("get surface capabilitites", vkGetPhysicalDeviceSurfaceCapabilities2KHR(physical_device, &surface_info, &mSurfaceCapabilities));
}

Allocator::Allocator(const VkInstance& instance, const VkPhysicalDevice& physical_device, const VkDevice& device)
{
	const VmaAllocatorCreateInfo allocator_create_info = {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = physical_device,
		.device = device,
		.instance = instance,
		.vulkanApiVersion = VK_MAKE_API_VERSION(0, 1, 4, 328),
	};

	VK_CHECK("create vma allocator", vmaCreateAllocator(&allocator_create_info, &mAllocator));
}

VmaAllocator Allocator::GetAllocator() const
{
	return mAllocator;
}

Allocator::~Allocator() noexcept
{
	vmaDestroyAllocator(mAllocator);
}

Swapchain::Swapchain(const VkDevice device, const Surface* surface_data, const uint32_t graphics_queue_family_index, const std::string& name) : mDevice(device)
{
	VkSwapchainCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface_data->GetSurfaceKHR(),
		.minImageCount = surface_data->GetSurfaceCapabilities().surfaceCapabilities.minImageCount,
		.imageFormat = surface_data->GetSurfaceFormat().format,
		.imageColorSpace = surface_data->GetSurfaceFormat().colorSpace,
		.imageExtent = surface_data->GetSurfaceCapabilities().surfaceCapabilities.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = surface_data->GetSurfaceCapabilities().surfaceCapabilities.supportedUsageFlags,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &graphics_queue_family_index,
		.preTransform = surface_data->GetSurfaceCapabilities().surfaceCapabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = surface_data->GetPresentMode(),
		.oldSwapchain = VK_NULL_HANDLE,
	};

	VK_CHECK("create swapchain", vkCreateSwapchainKHR(device, &create_info, nullptr, &mSwapchain));
	VK_CHECK("get swapchain images", vkGetSwapchainImagesKHR(device, mSwapchain, &mImagesCount, nullptr));

	mImages.resize(mImagesCount);
	VK_CHECK("get swapchain images", vkGetSwapchainImagesKHR(device, mSwapchain, &mImagesCount, mImages.data()));

	VkImageViewCreateInfo image_view_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = surface_data->GetSurfaceFormat().format,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};

	mImageViews.resize(mImagesCount);
	for (uint32_t i = 0; i < mImagesCount; ++i)
	{
		image_view_create_info.image = mImages[i];
		VK_CHECK("create swapchain image view", vkCreateImageView(device, &image_view_create_info, nullptr, &mImageViews[i]));
	}

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_SWAPCHAIN_KHR, reinterpret_cast<uint64_t>(mSwapchain), "swapchain");

	for (uint32_t i = 0; i < mImagesCount; ++i)
	{
		Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(mImages[i]), std::string("swapchain image ").append(std::to_string(i)).c_str());
		Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64_t>(mImageViews[i]), std::string("swapchain image view ").append(std::to_string(i)).c_str());
	}
#endif
}

Swapchain::~Swapchain() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
	{
		for (uint32_t i = 0; i < mImagesCount; ++i)
		{
			vkDestroyImageView(mDevice, mImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
	}
}

VkSwapchainKHR Swapchain::GetSwapchain() const
{
	return mSwapchain;
}

std::vector<VkImage> Swapchain::GetImages() const
{
	return mImages;
}

std::vector<VkImageView> Swapchain::GetImageViews() const
{
	return mImageViews;
}

uint32_t Swapchain::GetImagesCount() const
{
	return mImagesCount;
}

TransferHelpers::TransferHelpers(const VkDevice device, const VkQueue transfer_queue, const uint32_t transfer_queue_family_index, const std::string& name)
	:mQueue(transfer_queue), mQueueFamilyIndex(transfer_queue_family_index), mDevice(device)
{
	const VkCommandPoolCreateInfo cmd_pool_ci = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = transfer_queue_family_index,
	};

	VK_CHECK("create transfer cmd pool", vkCreateCommandPool(device, &cmd_pool_ci, nullptr, &mCommandPool));

	const VkCommandBufferAllocateInfo cmd_buff_ai = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = mCommandPool,
		.commandBufferCount = 1
	};

	VK_CHECK("allocate transfer cmd buff", vkAllocateCommandBuffers(device, &cmd_buff_ai, &mCommandBuffer));

	const VkSemaphoreTypeCreateInfo sem_tl_ci = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR,
		.initialValue = 0,
	};

	const VkSemaphoreCreateInfo sem_ci = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = &sem_tl_ci,
	};

	VK_CHECK("create transfer sem", vkCreateSemaphore(device, &sem_ci, nullptr, &mSemaphore));

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<uint64_t>(mCommandPool), "tranfer objects command pool");
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64_t>(mCommandBuffer), "tranfer objects command buffer");
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(mSemaphore), "transfer objects semaphore");
#endif
}

TransferHelpers::~TransferHelpers() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
		vkDestroySemaphore(mDevice, mSemaphore, nullptr);
	}
}

void TransferHelpers::BeginBatch()
{
	const VkSemaphoreWaitInfo  wait_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		.semaphoreCount = 1,
		.pSemaphores = &mSemaphore,
		.pValues = &mSemaphoreValue,
	};
	VK_CHECK("wait xfer sem", vkWaitSemaphoresKHR(mDevice, &wait_info, UINT64_MAX));

	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VK_CHECK("begin xfer cmd buff", vkBeginCommandBuffer(mCommandBuffer, &begin_info));
}

void TransferHelpers::ChangeImageLayout(const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask, const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask, const VkImageLayout old_layout, const VkImageLayout new_layout, const uint32_t src_q_fly_idx, const uint32_t dst_q_fly_idx, const VkImageAspectFlags aspect_mask, const VkImage& image)
{
	const VkImageMemoryBarrier2 img_mem_barr = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = src_stage_mask,
		.srcAccessMask = src_access_mask,
		.dstStageMask = dst_stage_mask,
		.dstAccessMask = dst_access_mask,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = src_q_fly_idx,
		.dstQueueFamilyIndex = dst_q_fly_idx,
		.image = image,
		.subresourceRange = {
			.aspectMask = aspect_mask,
			.levelCount = 1,
			.layerCount = 1,
		},
	};

	const VkDependencyInfo dep_info = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &img_mem_barr,
	};

	vkCmdPipelineBarrier2KHR(mCommandBuffer, &dep_info);
}

void TransferHelpers::CopyBufferToBuffer(const VkBuffer src_buffer, const VkBuffer dst_buffer, const VkDeviceSize size)
{
	const VkBufferCopy2 regions[] = {
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
			.size = size,
		},
	};

	const VkCopyBufferInfo2 copy_buff_info = {
		.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
		.srcBuffer = src_buffer,
		.dstBuffer = dst_buffer,
		.regionCount = std::size(regions),
		.pRegions = regions,
	};

	vkCmdCopyBuffer2KHR(mCommandBuffer, &copy_buff_info);
}

void TransferHelpers::CopyBufferToImage(const VkBuffer src_buffer, const VkImage dst_image, const VkExtent2D extent)
{
	const VkBufferImageCopy2KHR regions[] = {
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2_KHR,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.layerCount = 1,
			},
			.imageExtent = {
				.width = extent.width,
				.height = extent.height,
				.depth = 1,
			},
		},
	};

	const VkCopyBufferToImageInfo2KHR copy_buff_info = {
		.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2_KHR,
		.srcBuffer = src_buffer,
		.dstImage = dst_image,
		.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL,
		.regionCount = std::size(regions),
		.pRegions = regions,
	};

	vkCmdCopyBufferToImage2KHR(mCommandBuffer, &copy_buff_info);
}

void TransferHelpers::EndBatch()
{
	VK_CHECK("end xfer cmd buff", vkEndCommandBuffer(mCommandBuffer));

	const VkSemaphoreSubmitInfo wait_sem_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = mSemaphore,
			.value = mSemaphoreValue,
			.stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		}
	};

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = mCommandBuffer,
		},
	};

	const VkSemaphoreSubmitInfo sig_sem_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = mSemaphore,
			.value = ++mSemaphoreValue,
			.stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		},
	};

	const VkSubmitInfo2 submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = std::size(wait_sem_infos),
			.pWaitSemaphoreInfos = wait_sem_infos,
			.commandBufferInfoCount = std::size(cmd_buff_infos),
			.pCommandBufferInfos = cmd_buff_infos,
			.signalSemaphoreInfoCount = std::size(sig_sem_infos),
			.pSignalSemaphoreInfos = sig_sem_infos,
		},
	};

	VK_CHECK("submit xfer batch", vkQueueSubmit2KHR(mQueue, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));
}

VkCommandPool TransferHelpers::GetCommandPool() const
{
	return mCommandPool;
}

VkCommandBuffer TransferHelpers::GetCommandBuffer() const
{
	return mCommandBuffer;
}

VkQueue TransferHelpers::GetQueue() const
{
	return mQueue;
}

VkSemaphore TransferHelpers::GetSemaphore() const
{
	return mSemaphore;
}

uint64_t& TransferHelpers::GetSemaphoreValue()
{
	return mSemaphoreValue;
}

const uint64_t TransferHelpers::GetSemaphoreValueConst() const
{
	return mSemaphoreValue;
}

uint32_t TransferHelpers::GetQueueFamilyIndex() const
{
	return mQueueFamilyIndex;
}

Device::Device(const PhysicalDeviceData* physical_device_data)
{
	std::vector<const char*> req_ext_names = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MAINTENANCE_6_EXTENSION_NAME,
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
		VK_KHR_SPIRV_1_4_EXTENSION_NAME,
		VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
		VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
		VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
		VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
		VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
		VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME,
		"VK_KHR_maintenance5",
		"VK_KHR_maintenance6",
		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
		VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
		VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
	};

	uint32_t property_count = 0;
	VK_CHECK("enumerate device extensions", vkEnumerateDeviceExtensionProperties(physical_device_data->PhysicalDevice, nullptr, &property_count, nullptr));

	std::vector<VkExtensionProperties> props(property_count);
	VK_CHECK("enumerate device extensions", vkEnumerateDeviceExtensionProperties(physical_device_data->PhysicalDevice, nullptr, &property_count, props.data()));

	for (auto const& req_ext_name : req_ext_names)
	{
		auto it = std::find_if(props.begin(), props.end(), [&](const VkExtensionProperties prop)
			{ return (std::strcmp(prop.extensionName, req_ext_name) == 0); });

		if (it == props.end())
		{
			std::println("Extension {} is not supporeted by device", req_ext_name);
		}
	}

	VkDeviceQueueCreateInfo q_ci = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = 0,
		.queueCount = 1,
	};

	std::vector<VkDeviceQueueCreateInfo> d_q_cis{ q_ci };
	{
		auto d_q_ci_it = std::find_if(std::begin(d_q_cis), std::end(d_q_cis), [&](const VkDeviceQueueCreateInfo d_q_ci) {return d_q_ci.queueFamilyIndex == physical_device_data->TransferQueueFamilyIndex; });

		if (d_q_ci_it == std::end(d_q_cis))
		{
			q_ci.queueFamilyIndex = physical_device_data->TransferQueueFamilyIndex;
			q_ci.queueCount = 1;
			d_q_cis.push_back(q_ci);
		}
		else
		{
			++d_q_ci_it->queueCount;
		}

		d_q_ci_it = std::find_if(std::begin(d_q_cis), std::end(d_q_cis), [&](const VkDeviceQueueCreateInfo d_q_ci) {return d_q_ci.queueFamilyIndex == physical_device_data->ComputeQueueFamilyIndex; });

		if (d_q_ci_it == std::end(d_q_cis))
		{
			q_ci.queueFamilyIndex = physical_device_data->ComputeQueueFamilyIndex;
			q_ci.queueCount = 1;
			d_q_cis.push_back(q_ci);
		}
		else
		{
			++d_q_ci_it->queueCount;
		}
	}

	std::vector<std::vector<float>> priorities(d_q_cis.size());
	{
		size_t d_q_ci_idx = 0;
		for (auto& d_q_ci : d_q_cis)
		{
			priorities[d_q_ci_idx].resize(d_q_ci.queueCount, 1.f);
			d_q_ci.pQueuePriorities = priorities[d_q_ci_idx].data();
		}
	}

	VkPhysicalDeviceRobustness2FeaturesEXT rob2_feats = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
	};

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipe_feats = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
		.pNext = &rob2_feats,
	};

	VkPhysicalDeviceAccelerationStructureFeaturesKHR accel_struct_feats = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
		.pNext = &rt_pipe_feats,
	};

	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dyn_feats = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
		.pNext = &accel_struct_feats,
	};

	VkPhysicalDeviceDynamicRenderingFeatures dyn_rend_feats = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
		.pNext = &extended_dyn_feats,
	};

	VkPhysicalDeviceSynchronization2Features sync2_feats = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
		.pNext = &dyn_rend_feats,
	};

	VkPhysicalDeviceVulkan12Features feats12 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &sync2_feats,
	};

	VkPhysicalDeviceFeatures2 feats2 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &feats12,
	};

	vkGetPhysicalDeviceFeatures2(physical_device_data->PhysicalDevice, &feats2);

	const VkDeviceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &feats2,
		.queueCreateInfoCount = static_cast<uint32_t>(d_q_cis.size()),
		.pQueueCreateInfos = d_q_cis.data(),
		.enabledExtensionCount = static_cast<uint32_t>(req_ext_names.size()),
		.ppEnabledExtensionNames = req_ext_names.data(),
	};

	VK_CHECK("create device", vkCreateDevice(physical_device_data->PhysicalDevice, &create_info, nullptr, &mDevice));

	vk_CmdPipelineBarrier2KHR = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(vkGetDeviceProcAddr(mDevice, "vkCmdPipelineBarrier2KHR"));
	vk_QueueSubmit2KHR = reinterpret_cast<PFN_vkQueueSubmit2KHR>(vkGetDeviceProcAddr(mDevice, "vkQueueSubmit2KHR"));
	vk_SignalSemaphoreKHR = reinterpret_cast<PFN_vkSignalSemaphoreKHR>(vkGetDeviceProcAddr(mDevice, "vkSignalSemaphoreKHR"));
	vk_CmdPipelineBarrier2KHR = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(vkGetDeviceProcAddr(mDevice, "vkCmdPipelineBarrier2KHR"));
	vk_CmdCopyBuffer2KHR = reinterpret_cast<PFN_vkCmdCopyBuffer2KHR>(vkGetDeviceProcAddr(mDevice, "vkCmdCopyBuffer2KHR"));
	vk_WaitSemaphoresKHR = reinterpret_cast<PFN_vkWaitSemaphoresKHR>(vkGetDeviceProcAddr(mDevice, "vkWaitSemaphoresKHR"));
	vk_CmdBeginRenderingKHR = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(mDevice, "vkCmdBeginRenderingKHR"));
	vk_CmdBindDescriptorSets2KHR = reinterpret_cast<PFN_vkCmdBindDescriptorSets2KHR>(vkGetDeviceProcAddr(mDevice, "vkCmdBindDescriptorSets2KHR"));
	vk_CmdBindVertexBuffers2EXT = reinterpret_cast<PFN_vkCmdBindVertexBuffers2EXT>(vkGetDeviceProcAddr(mDevice, "vkCmdBindVertexBuffers2EXT"));
	vk_CmdCopyBufferToImage2KHR = reinterpret_cast<PFN_vkCmdCopyBufferToImage2KHR>(vkGetDeviceProcAddr(mDevice, "vkCmdCopyBufferToImage2KHR"));
	vk_CmdBindIndexBuffer2KHR = reinterpret_cast<PFN_vkCmdBindIndexBuffer2KHR>(vkGetDeviceProcAddr(mDevice, "vkCmdBindIndexBuffer2KHR"));
	vk_CmdPushConstants2KHR = reinterpret_cast<PFN_vkCmdPushConstants2KHR>(vkGetDeviceProcAddr(mDevice, "vkCmdPushConstants2KHR"));
	vk_CmdEndRenderingKHR = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(mDevice, "vkCmdEndRenderingKHR"));
	vk_GetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(mDevice, "vkGetRayTracingShaderGroupHandlesKHR"));
	vk_CreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(mDevice, "vkCreateRayTracingPipelinesKHR"));
	vk_CmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(mDevice, "vkCmdTraceRaysKHR"));
	vk_GetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(mDevice, "vkGetBufferDeviceAddressKHR"));
	vk_CreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(mDevice, "vkCreateAccelerationStructureKHR"));
	vk_DestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(mDevice, "vkDestroyAccelerationStructureKHR"));
	vk_CmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(mDevice, "vkCmdBuildAccelerationStructuresKHR"));
	vk_GetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(mDevice, "vkGetAccelerationStructureBuildSizesKHR"));
	vk_GetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(mDevice, "vkGetAccelerationStructureDeviceAddressKHR"));

	VkDeviceQueueInfo2 queue_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
	};

	for (const auto& d_q_ci : d_q_cis)
	{
		for (uint32_t q = 0; q < d_q_ci.queueCount; ++q)
		{
			if (d_q_ci.queueFamilyIndex == physical_device_data->GraphicsQueueFamilyIndex)
			{
				queue_info.queueFamilyIndex = physical_device_data->GraphicsQueueFamilyIndex;
				queue_info.queueIndex = q;
				vkGetDeviceQueue2(mDevice, &queue_info, &mGraphicsQueue);
			}
			else if (d_q_ci.queueFamilyIndex == physical_device_data->TransferQueueFamilyIndex)
			{
				queue_info.queueFamilyIndex = physical_device_data->TransferQueueFamilyIndex;
				queue_info.queueIndex = q;
				vkGetDeviceQueue2(mDevice, &queue_info, &mTransferQueue);
			}
			else if (d_q_ci.queueFamilyIndex == physical_device_data->ComputeQueueFamilyIndex)
			{
				queue_info.queueFamilyIndex = physical_device_data->ComputeQueueFamilyIndex;
				queue_info.queueIndex = q;
				vkGetDeviceQueue2(mDevice, &queue_info, &mComputeQueue);
			}
		}
	}

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_DEVICE, reinterpret_cast<uint64_t>(mDevice), "device");
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_PHYSICAL_DEVICE, reinterpret_cast<uint64_t>(physical_device_data->PhysicalDevice), physical_device_data->Properties.properties.deviceName);
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_QUEUE, reinterpret_cast<uint64_t>(mGraphicsQueue), "graphics queue");
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_QUEUE, reinterpret_cast<uint64_t>(mComputeQueue), "compute queue");
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_QUEUE, reinterpret_cast<uint64_t>(mTransferQueue), "transfer queue");

#endif	// _DEBUG
}

Device::~Device() noexcept
{
	vkDestroyDevice(mDevice, nullptr);
}

VkDevice Device::GetDevice() const
{
	return mDevice;
}

VkQueue Device::GetGraphicsQueue() const
{
	return mGraphicsQueue;
}

VkQueue Device::GetComputeQueue() const
{
	return mComputeQueue;
}

VkQueue Device::GetTransferQueue() const
{
	return mTransferQueue;
}

AccelerationStructure::AccelerationStructure(const VkDevice device, const VmaAllocator allocator, const VkCommandBuffer command_buffer, const VkQueue queue)
{
}

AccelerationStructure::~AccelerationStructure() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
		vkDestroyAccelerationStructureKHR(mDevice, mAccelerationStructure, nullptr);
}

ComputeHelpers::ComputeHelpers(const VkDevice device, const VkQueue compute_queue, const uint32_t compute_queue_family_index, const std::string& name)
	:mDevice(device), mQueue(compute_queue)
{
	const VkCommandPoolCreateInfo cmd_pool_ci = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = compute_queue_family_index,
	};

	VK_CHECK("create compute cmd pool", vkCreateCommandPool(device, &cmd_pool_ci, nullptr, &mCommandPool));

	const VkCommandBufferAllocateInfo cmd_buff_ai = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = mCommandPool,
		.commandBufferCount = 1
	};

	VK_CHECK("allocate compute cmd buff", vkAllocateCommandBuffers(device, &cmd_buff_ai, &mCommandBuffer));

	const VkSemaphoreTypeCreateInfo sem_tl_ci = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR,
		.initialValue = 0,
	};

	const VkSemaphoreCreateInfo sem_ci = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = &sem_tl_ci,
	};

	VK_CHECK("create compute sem", vkCreateSemaphore(device, &sem_ci, nullptr, &mSemaphore));

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<uint64_t>(mCommandPool), "compute helpers command pool");
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64_t>(mCommandBuffer), "compute helpers command buffer");
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(mSemaphore), "compute helpers semaphore");
#endif
}

ComputeHelpers::~ComputeHelpers() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
		vkDestroySemaphore(mDevice, mSemaphore, nullptr);
	}
}

VkCommandPool ComputeHelpers::GetCommandPool() const
{
	return mCommandPool;
}

VkCommandBuffer ComputeHelpers::GetCommandBuffer() const
{
	return mCommandBuffer;
}

VkQueue ComputeHelpers::GetQueue() const
{
	return mQueue;
}

VkSemaphore ComputeHelpers::GetSemaphore() const
{
	return mSemaphore;
}

uint64_t& ComputeHelpers::GetSemaphoreValue()
{
	return mSemaphoreValue;
}

uint32_t ComputeHelpers::GetQueueFamilyIndex() const
{
	return mQueueFamilyIndex;
}

