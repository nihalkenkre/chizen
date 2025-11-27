#include "vulkan_objects.hpp"
#include "utils.hpp"
#include "vulkan_interface.hpp"

PFN_vkSetDebugUtilsObjectNameEXT vk_SetDebugUtilsObjectNameEXT = nullptr;
PFN_vkGetRayTracingShaderGroupHandlesKHR vk_GetRayTracingShaderGroupHandlesKHR = nullptr;
PFN_vkCreateRayTracingPipelinesKHR vk_CreateRayTracingPipelinesKHR = nullptr;
PFN_vkCmdTraceRaysKHR vk_CmdTraceRaysKHR = nullptr;

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(
	VkDevice                                    device,
	const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
	return vk_SetDebugUtilsObjectNameEXT(device, pNameInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesKHR(
	VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount,
	size_t dataSize, void* pData)
{
	return vk_GetRayTracingShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesKHR(
	VkDevice device, VkDeferredOperationKHR deferredOperation,
	VkPipelineCache pipelineCache, uint32_t createInfoCount,
	const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
	const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	return vk_CreateRayTracingPipelinesKHR(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysKHR(
	VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
	const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
	const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, uint32_t width, uint32_t height, uint32_t depth)
{
	return vk_CmdTraceRaysKHR(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
}
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
		.apiVersion = VK_MAKE_API_VERSION(0, 1, 4, 328),
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
	//SDL_Vulkan_DestroySurface(mInstance, mSurface, nullptr);
	if (mInstance != VK_NULL_HANDLE)
		vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
}

VkSurfaceKHR Surface::GetSurface() const
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
		.surface = surface_data->GetSurface(),
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
	VkDebugUtilsObjectNameInfoEXT name_info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR,
		.objectHandle = reinterpret_cast<uint64_t>(mSwapchain),
		.pObjectName = name.c_str(),
	};

	VK_CHECK("set swapchain name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

	std::string n;
	for (uint32_t i = 0; i < mImagesCount; ++i)
	{
		n = name;
		name_info.objectType = VK_OBJECT_TYPE_IMAGE;
		name_info.objectHandle = reinterpret_cast<uint64_t>(mImages[i]);
		name_info.pObjectName = n.append(" image ").append(std::to_string(i)).c_str();
		VK_CHECK("setting swapchain image name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		n = name;
		name_info.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
		name_info.objectHandle = reinterpret_cast<uint64_t>(mImageViews[i]);
		name_info.pObjectName = n.append(" image view ").append(std::to_string(i)).c_str();
		VK_CHECK("setting swapcahin image view name", vkSetDebugUtilsObjectNameEXT(device, &name_info))
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

TransferObjects::TransferObjects(const VkDevice& device, const VkQueue& transfer_queue, const uint32_t transfer_queue_family_index)
{
	mQueue = transfer_queue;
	mQueueFamilyIndex = transfer_queue_family_index;
	mDevice = device;

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

}

TransferObjects::~TransferObjects() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
	}
}

void TransferObjects::PrepareImage(const VkImage& image)
{
	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	VK_CHECK("begin cmd buff", vkBeginCommandBuffer(mCommandBuffer, &begin_info));

	Utils_ChangeImageLayout(mCommandBuffer,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		image);

	const VkImageSubresourceRange ranges[] = {
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};

	const VkClearColorValue clear_color = {
		.float32 = {
			0, 0, 0, 1,
		},
	};

	vkCmdClearColorImage(mCommandBuffer, image, VK_IMAGE_LAYOUT_GENERAL, &clear_color, std::size(ranges), ranges);

	VK_CHECK("end emd buff", vkEndCommandBuffer(mCommandBuffer));

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = mCommandBuffer,
		},
	};

	const VkSubmitInfo2 submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = std::size(cmd_buff_infos),
			.pCommandBufferInfos = cmd_buff_infos,
		},
	};

	VK_CHECK("submit tranfer commands", vkQueueSubmit2(mQueue, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));
	VK_CHECK("queue wait idle", vkQueueWaitIdle(mQueue));
}

VkCommandPool TransferObjects::GetCommandPool() const
{
	return mCommandPool;
}

VkCommandBuffer TransferObjects::GetCommandBuffer() const
{
	return mCommandBuffer;
}

VkQueue TransferObjects::GetQueue() const
{
	return mQueue;
}

uint32_t TransferObjects::GetQueueFamilyIndex() const
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

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipe_feats = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
	};

	VkPhysicalDeviceAccelerationStructureFeaturesKHR accel_struct_feats = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
		.pNext = &rt_pipe_feats,
	};

	VkPhysicalDeviceDynamicRenderingFeatures dyn_rend_feats = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
		.pNext = &accel_struct_feats,
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

	vk_GetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(mDevice, "vkGetRayTracingShaderGroupHandlesKHR"));
	vk_CreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(mDevice, "vkCreateRayTracingPipelinesKHR"));
	vk_CmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(mDevice, "vkCmdTraceRaysKHR"));

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
	VkDebugUtilsObjectNameInfoEXT name_info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.objectType = VK_OBJECT_TYPE_DEVICE,
		.objectHandle = reinterpret_cast<uint64_t>(mDevice),
		.pObjectName = "device",
	};

	VK_CHECK("set device name", vkSetDebugUtilsObjectNameEXT(mDevice, &name_info));

	name_info.objectType = VK_OBJECT_TYPE_QUEUE;
	name_info.objectHandle = reinterpret_cast<uint64_t>(mGraphicsQueue);
	name_info.pObjectName = "graphics queue";
	VK_CHECK("set graphics queue name", vkSetDebugUtilsObjectNameEXT(mDevice, &name_info));

	name_info.objectType = VK_OBJECT_TYPE_QUEUE;
	name_info.objectHandle = reinterpret_cast<uint64_t>(mComputeQueue);
	name_info.pObjectName = "compute queue";

	VK_CHECK("set cmpt_q name", vkSetDebugUtilsObjectNameEXT(mDevice, &name_info));

	name_info.objectType = VK_OBJECT_TYPE_QUEUE;
	name_info.objectHandle = reinterpret_cast<uint64_t>(mTransferQueue);
	name_info.pObjectName = "transfer queue";

	VK_CHECK("set xfer_q name", vkSetDebugUtilsObjectNameEXT(mDevice, &name_info));

	name_info.objectType = VK_OBJECT_TYPE_PHYSICAL_DEVICE;
	name_info.objectHandle = reinterpret_cast<uint64_t>(physical_device_data->PhysicalDevice);
	name_info.pObjectName = physical_device_data->Properties.properties.deviceName;

	VK_CHECK("set physical device name", vkSetDebugUtilsObjectNameEXT(mDevice, &name_info));

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
