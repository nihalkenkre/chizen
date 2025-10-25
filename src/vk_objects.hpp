#pragma once

extern "C" PFN_vkSetDebugUtilsObjectNameEXT vk_SetDebugUtilsObjectNameEXT = nullptr;

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(
	VkDevice                                    device,
	const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
	return vk_SetDebugUtilsObjectNameEXT(device, pNameInfo);
}

#define VK_CHECK(action, result)					\
	if (result < VK_SUCCESS)						\
	{														\
		std::printf("%s %d\n", action, result);	\
	}

inline static VkDeviceSize ALIGNED_SIZE(VkDeviceSize value, VkDeviceSize alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

uint32_t get_memory_type_id(const VkPhysicalDeviceMemoryProperties2 mem_props, const VkMemoryRequirements2 mem_reqs, const uint32_t mem_prop_types)
{
	uint32_t mem_id = UINT32_MAX;

	for (uint32_t mt = 0; mt < mem_props.memoryProperties.memoryTypeCount; ++mt)
	{
		if (mem_props.memoryProperties.memoryTypes[mt].propertyFlags & mem_prop_types)
		{
			if (mem_reqs.memoryRequirements.memoryTypeBits & (1 << mt))
			{
				if (mem_props.memoryProperties.memoryHeaps[mem_props.memoryProperties.memoryTypes[mt].heapIndex].size > mem_reqs.memoryRequirements.size)
				{
					mem_id = mt;
					break;
				}
			}
		}
	}

	return mem_id;
}

void copy_buffer_to_buffer(const VkBuffer src_buffer, const VkBuffer dst_buffer, const std::vector<VkBufferCopy> regions, const VkCommandBuffer cmd_buff, const VkQueue xfer_q)
{
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK("begin command buffer", vkBeginCommandBuffer(cmd_buff, &begin_info));

	vkCmdCopyBuffer(cmd_buff, src_buffer, dst_buffer, static_cast<uint32_t>(regions.size()), regions.data());

	VK_CHECK("end command buffer", vkEndCommandBuffer(cmd_buff));

	const VkSubmitInfo submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &cmd_buff,
		},
	};

	VK_CHECK("queue submit", vkQueueSubmit(xfer_q, _countof(submit_infos), submit_infos, VK_NULL_HANDLE));
	VK_CHECK("queue wait idle", vkQueueWaitIdle(xfer_q));
	VK_CHECK("reset buffer copy cmd buff", vkResetCommandBuffer(cmd_buff, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
}

void copy_buffer_to_image(const VkBuffer src_buffer, const VkImage dst_image, const VkImageLayout dst_image_layout, const std::vector<VkBufferImageCopy2>& regions, const VkCommandBuffer cmd_buff, const VkQueue xfer_q)
{
	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	VK_CHECK("begin command buffer", vkBeginCommandBuffer(cmd_buff, &begin_info));

	const VkImageMemoryBarrier2 img_copy_bar = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.image = dst_image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		}
	};

	VkDependencyInfo dependency_info = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &img_copy_bar,
	};
	vkCmdPipelineBarrier2(cmd_buff, &dependency_info);

	const VkCopyBufferToImageInfo2 copy_info = {
		.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
		.srcBuffer = src_buffer,
		.dstImage = dst_image,
		.dstImageLayout = dst_image_layout,
		.regionCount = static_cast<uint32_t>(regions.size()),
		.pRegions = regions.data(),
	};
	vkCmdCopyBufferToImage2(cmd_buff, &copy_info);

	const VkImageMemoryBarrier2 img_lyt_chng_bar = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		.dstAccessMask = 0,
		.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.image = dst_image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		}
	};

	dependency_info.pImageMemoryBarriers = &img_lyt_chng_bar;
	vkCmdPipelineBarrier2(cmd_buff, &dependency_info);

	VK_CHECK("end command buffer", vkEndCommandBuffer(cmd_buff));

	const VkSubmitInfo submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &cmd_buff,
		}
	};

	VK_CHECK("queue submit", vkQueueSubmit(xfer_q, _countof(submit_infos), submit_infos, VK_NULL_HANDLE));
	VK_CHECK("queue wait idle", vkQueueWaitIdle(xfer_q));
	VK_CHECK("reset buffer copy cmd buff", vkResetCommandBuffer(cmd_buff, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
}

namespace vk_instance
{
	VkInstance create(const char* const* extensions, uint32_t extensions_count)
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
			.apiVersion = VK_MAKE_API_VERSION(0, 1, 4, 304),
		};

		const VkInstanceCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &app_info,
			.enabledExtensionCount = static_cast<uint32_t>(req_ext_names.size()),
			.ppEnabledExtensionNames = req_ext_names.data(),
		};

		VkInstance instance = VK_NULL_HANDLE;
		VK_CHECK("create instance", vkCreateInstance(&create_info, nullptr, &instance));

		return instance;
	}

	void destroy(const VkInstance instance)
	{
		if (instance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(instance, nullptr);
		}
	}
}

namespace vk_surface
{
	struct data
	{
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkSurfaceCapabilitiesKHR surf_caps = {};
		VkPresentModeKHR present_mode = {};
		VkSurfaceFormatKHR format = {};
	};
}

namespace vk_phydev
{
	struct data
	{
		VkPhysicalDevice phy_dev = VK_NULL_HANDLE;
		uint32_t q_count = 0;
		uint32_t q_fly_idx = 0;
		VkPhysicalDeviceProperties2 props = {};
		VkPhysicalDeviceMemoryProperties2 mem_props = {};
		VkPhysicalDeviceDescriptorBufferPropertiesEXT desc_buff_props = {};
	};

	data get_phy_dev(const VkInstance instance, vk_surface::data* surface)
	{
		uint32_t phy_dev_count = 0;
		VK_CHECK("enumerate physical devices", vkEnumeratePhysicalDevices(instance, &phy_dev_count, nullptr));

		std::vector<VkPhysicalDevice> phy_devs(phy_dev_count);
		VK_CHECK("enumerate physical devices", vkEnumeratePhysicalDevices(instance, &phy_dev_count, phy_devs.data()));

		vk_phydev::data d;
		d.props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		d.mem_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;

		for (auto const& phy_dev : phy_devs)
		{
			d.desc_buff_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;

			VkPhysicalDeviceProperties2 props = {};
			props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

			vkGetPhysicalDeviceProperties2(phy_dev, &props);

			if (props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				uint32_t q_fly_count = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(phy_dev, &q_fly_count, nullptr);

				std::vector<VkQueueFamilyProperties> q_fly_props(q_fly_count);
				vkGetPhysicalDeviceQueueFamilyProperties(phy_dev, &q_fly_count, q_fly_props.data());

				for (uint32_t q = 0; q < q_fly_count; ++q)
				{
					VkBool32 is_supported = false;
					vkGetPhysicalDeviceSurfaceSupportKHR(phy_dev, q, surface->surface, &is_supported);

					if (is_supported &&
						SDL_Vulkan_GetPresentationSupport(instance, phy_dev, q) &&
						q_fly_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					{
						d.phy_dev = phy_dev;
						d.q_count = q_fly_props[q].queueCount;
						d.q_fly_idx = q;
						d.props = props;

						vkGetPhysicalDeviceMemoryProperties2(phy_dev, &d.mem_props);

						VK_CHECK("get surface caps", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phy_dev, surface->surface, &surface->surf_caps));

						{
							uint32_t surf_forms_count = 0;
							VK_CHECK("get surface formats", vkGetPhysicalDeviceSurfaceFormatsKHR(phy_dev, surface->surface, &surf_forms_count, nullptr));

							std::vector<VkSurfaceFormatKHR> surf_forms(surf_forms_count);
							VK_CHECK("get surface formats", vkGetPhysicalDeviceSurfaceFormatsKHR(phy_dev, surface->surface, &surf_forms_count, surf_forms.data()));

							auto it = std::find_if(surf_forms.begin(), surf_forms.end(), [](const VkSurfaceFormatKHR frm)
								{ return (frm.format == VK_FORMAT_R8G8B8A8_UNORM) && (frm.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR); });

							if (it != surf_forms.end())
							{
								surface->format = *it;
							}
						}

						{
							uint32_t present_modes_count = 0;
							VK_CHECK("get present modes", vkGetPhysicalDeviceSurfacePresentModesKHR(phy_dev, surface->surface, &present_modes_count, nullptr));

							std::vector<VkPresentModeKHR> present_modes(present_modes_count);
							VK_CHECK("get surface formats", vkGetPhysicalDeviceSurfacePresentModesKHR(phy_dev, surface->surface, &present_modes_count, present_modes.data()));

							auto it = std::find_if(present_modes.begin(), present_modes.end(), [](const VkPresentModeKHR mode)
								{ return mode == VK_PRESENT_MODE_MAILBOX_KHR; });

							if (it != present_modes.end())
							{
								surface->present_mode = *it;
							}
						}
					}
				}
			}
		}

		return d;
	}
}
namespace vk_device
{
	struct data
	{
		VkDevice device = VK_NULL_HANDLE;
		VkQueue gfx_q = VK_NULL_HANDLE;
		VkQueue xfer_q = VK_NULL_HANDLE;
	};

	data create(const vk_phydev::data phy_dev_data, const uint32_t q_fly_idx, const uint32_t q_count)
	{
		std::vector<const char*> req_ext_names = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
			VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
		};

		uint32_t property_count = 0;
		VK_CHECK("enumerate device extensions", vkEnumerateDeviceExtensionProperties(phy_dev_data.phy_dev, nullptr, &property_count, nullptr));

		std::vector<VkExtensionProperties> props(property_count);
		VK_CHECK("enumerate device extensions", vkEnumerateDeviceExtensionProperties(phy_dev_data.phy_dev, nullptr, &property_count, props.data()));

		for (auto const& req_ext_name : req_ext_names)
		{
			auto it = std::find_if(props.begin(), props.end(), [&](const VkExtensionProperties prop)
				{ return (std::strcmp(prop.extensionName, req_ext_name) == 0); });

			if (it == props.end())
			{
				std::println("Extension {} is not supporeted by device", req_ext_name);
			}
		}

		std::vector<float> priorities(q_count, 1);
		const VkDeviceQueueCreateInfo q_create_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = q_fly_idx,
			.queueCount = q_count,
			.pQueuePriorities = priorities.data(),
		};

		VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchain_main_1_feats = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT,
			.pNext = nullptr,
		};

		VkPhysicalDeviceDynamicRenderingFeatures dyn_rend_feats = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
			.pNext = &swapchain_main_1_feats,
		};

		VkPhysicalDeviceSynchronization2Features sync2_feats = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
			.pNext = &dyn_rend_feats,
		};

		VkPhysicalDeviceTimelineSemaphoreFeatures time_sem_feats = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
			.pNext = &sync2_feats,
		};

		VkPhysicalDeviceFeatures2 feats2 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
			.pNext = &time_sem_feats,
		};

		vkGetPhysicalDeviceFeatures2(phy_dev_data.phy_dev, &feats2);

		const VkDeviceCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = &feats2,
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &q_create_info,
			.enabledExtensionCount = static_cast<uint32_t>(req_ext_names.size()),
			.ppEnabledExtensionNames = req_ext_names.data(),
		};

		data d = {};

		VK_CHECK("create device", vkCreateDevice(phy_dev_data.phy_dev, &create_info, nullptr, &d.device));

		vk_SetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(d.device, "vkSetDebugUtilsObjectNameEXT"));

		VkDeviceQueueInfo2 queue_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
			.queueFamilyIndex = phy_dev_data.q_fly_idx,
			.queueIndex = 0,
		};
		vkGetDeviceQueue2(d.device, &queue_info, &d.gfx_q);

		queue_info.queueIndex = 1;
		vkGetDeviceQueue2(d.device, &queue_info, &d.xfer_q);

		return d;
	}

	void destroy(const VkDevice device)
	{
		if (device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(device, nullptr);
		}
	}
}
namespace vk_swapchain
{
	struct data
	{
		VkSwapchainKHR swapchain = VK_NULL_HANDLE;
		VkCommandPool cmd_pool = VK_NULL_HANDLE;

		std::vector<VkImage> images;
		std::vector<VkImageView> image_views;
		std::vector<VkCommandBuffer> cmd_buffs;
		std::vector<VkSemaphore> rndr_semaphores;
		std::vector<VkFence> present_fences;
		uint32_t images_count = 0;
		uint32_t curr_img_idx = 0;
	};

	data create(const VkDevice device, const vk_surface::data& surface, const vk_phydev::data& phy_dev, const VkSwapchainKHR old_swapchain, const std::string& name)
	{
		VkSwapchainCreateInfoKHR create_info = {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = surface.surface,
			.minImageCount = surface.surf_caps.minImageCount,
			.imageFormat = surface.format.format,
			.imageColorSpace = surface.format.colorSpace,
			.imageExtent = surface.surf_caps.currentExtent,
			.imageArrayLayers = 1,
			.imageUsage = surface.surf_caps.supportedUsageFlags,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = &phy_dev.q_fly_idx,
			.preTransform = surface.surf_caps.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = surface.present_mode,
			.oldSwapchain = old_swapchain,
		};

		vk_swapchain::data d;
		VK_CHECK("create swapchain", vkCreateSwapchainKHR(device, &create_info, nullptr, &d.swapchain));
		VK_CHECK("get swapchain images", vkGetSwapchainImagesKHR(device, d.swapchain, &d.images_count, nullptr));

		d.images.resize(d.images_count);
		VK_CHECK("get swapchain images", vkGetSwapchainImagesKHR(device, d.swapchain, &d.images_count, d.images.data()));

		d.image_views.resize(d.images_count);
		d.cmd_buffs.resize(d.images_count);
		d.rndr_semaphores.resize(d.images_count);
		d.present_fences.resize(d.images_count);

		VkImageViewCreateInfo image_view_create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = surface.format.format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_R,
				.g = VK_COMPONENT_SWIZZLE_G,
				.b = VK_COMPONENT_SWIZZLE_B,
				.a = VK_COMPONENT_SWIZZLE_A,
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			},
		};

		const VkCommandPoolCreateInfo cmd_pool_ci = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = phy_dev.q_fly_idx,
		};

		VK_CHECK("create command pool", vkCreateCommandPool(device, &cmd_pool_ci, nullptr, &d.cmd_pool));

		VkCommandBufferAllocateInfo cmd_buff_ai = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		const VkSemaphoreCreateInfo sem_ci = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};

		const VkFenceCreateInfo fence_ci = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};

		for (uint32_t i = 0; i < d.images_count; ++i)
		{
			image_view_create_info.image = d.images[i];
			VK_CHECK("create swapchain image view", vkCreateImageView(device, &image_view_create_info, nullptr, &d.image_views[i]));

			cmd_buff_ai.commandPool = d.cmd_pool;
			VK_CHECK("allocate command buffer", vkAllocateCommandBuffers(device, &cmd_buff_ai, &d.cmd_buffs[i]));
			VK_CHECK("create semaphore", vkCreateSemaphore(device, &sem_ci, nullptr, &d.rndr_semaphores[i]));
			VK_CHECK("create fence", vkCreateFence(device, &fence_ci, nullptr, &d.present_fences[i]));
		}

#ifdef _DEBUG
		VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR,
			.objectHandle = reinterpret_cast<uint64_t>(d.swapchain),
			.pObjectName = name.c_str(),
		};

		VK_CHECK("set swapchain name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		name_info.objectType = VK_OBJECT_TYPE_COMMAND_POOL;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.cmd_pool);
		name_info.pObjectName = "swapchain command pool";

		for (uint32_t i = 0; i < d.images_count; ++i)
		{
			std::string sc_img("swapchain image ");

			name_info.objectType = VK_OBJECT_TYPE_IMAGE;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.images[i]);
			name_info.pObjectName = sc_img.append(std::to_string(i)).c_str();// std::strcat(object_name, _itoa(i, i_str, 10));
			VK_CHECK("setting swapchain image name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

			std::string sc_iv("swapchain image view ");
			name_info.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.image_views[i]);
			name_info.pObjectName = sc_iv.append(std::to_string(i)).c_str();
			VK_CHECK("setting swapcahin image view name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

			std::string sc_cb("swapchain command buffer ");
			name_info.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.cmd_buffs[i]);
			name_info.pObjectName = sc_cb.append(std::to_string(i)).c_str();
			VK_CHECK("setting swapchain command buffer name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

			std::string sc_r_sem("swapchain render semaphore ");
			name_info.objectType = VK_OBJECT_TYPE_SEMAPHORE;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.rndr_semaphores[i]);
			name_info.pObjectName = sc_r_sem.append(std::to_string(i)).c_str();
			VK_CHECK("setting swapchain render sem name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

			std::string sc_pr_fnc("swapchain present fence ");
			name_info.objectType = VK_OBJECT_TYPE_FENCE;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.present_fences[i]);
			name_info.pObjectName = sc_pr_fnc.append(std::to_string(i)).c_str();
			VK_CHECK("setting swapchain present fence name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
		}
#endif // _DEBUG

		return d;
	}

	void destroy(vk_swapchain::data data, const VkDevice device)
	{
		vkDestroyCommandPool(device, data.cmd_pool, nullptr);

		for (uint32_t i = 0; i < data.images_count; ++i)
		{
			vkDestroyFence(device, data.present_fences[i], nullptr);
			vkDestroyImageView(device, data.image_views[i], nullptr);
			vkDestroySemaphore(device, data.rndr_semaphores[i], nullptr);
		}

		if (data.swapchain != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(device, data.swapchain, nullptr);
		}
	}
};

namespace vk_command_pool
{
	struct data {
		VkCommandPool cmd_pool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> cmd_buffs;
	};

	data create(const VkDevice device, const uint32_t q_fly_idx, const uint32_t cmd_buffs_count, const std::string& name)
	{
		const VkCommandPoolCreateInfo cmd_pool_ci = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = q_fly_idx,
		};

		vk_command_pool::data d;

		VK_CHECK("create command pool", vkCreateCommandPool(device, &cmd_pool_ci, nullptr, &d.cmd_pool));

		const VkCommandBufferAllocateInfo allocate_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = d.cmd_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = cmd_buffs_count,
		};

		d.cmd_buffs.resize(cmd_buffs_count);

		VK_CHECK("allocate command buffer", vkAllocateCommandBuffers(device, &allocate_info, d.cmd_buffs.data()));

#ifdef _DEBUG
		VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_COMMAND_POOL,
			.objectHandle = reinterpret_cast<uint64_t>(d.cmd_pool),
			.pObjectName = name.c_str(),
		};

		VK_CHECK("setting command pool name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		for (size_t cb = 0; cb < d.cmd_buffs.size(); ++cb)
		{
			name_info.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.cmd_buffs[cb]);
			name_info.pObjectName = std::string(name).append(" command buffer ").append(std::to_string(cb)).c_str();

			VK_CHECK("setting command pool name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
		}
#endif // _DEBUG

		return d;
	}

	void destroy(vk_command_pool::data cmd_pool, const VkDevice device)
	{
		if (cmd_pool.cmd_pool != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(device, cmd_pool.cmd_pool, nullptr);
		}
	}
}

namespace vk_command_buffer
{
	VkCommandBuffer allocate(const VkDevice device, const VkCommandPool cmd_pool, const std::string& name)
	{
		const VkCommandBufferAllocateInfo allocate_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = cmd_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		VkCommandBuffer cmd_buff = VK_NULL_HANDLE;
		VK_CHECK("allocate command buffer", vkAllocateCommandBuffers(device, &allocate_info, &cmd_buff));

#ifdef _DEBUG
		const VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER,
			.objectHandle = reinterpret_cast<uint64_t>(cmd_buff),
			.pObjectName = name.c_str(),
		};

		VK_CHECK("setting command buffer name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // _DEBUG

		return cmd_buff;
	}
}

namespace vk_semaphore
{
	struct data
	{
		VkSemaphore semaphore = VK_NULL_HANDLE;
		VkSemaphoreType type = VK_SEMAPHORE_TYPE_BINARY;
	};

	vk_semaphore::data create(const VkDevice device, const VkSemaphoreType semaphore_type, const std::string& name)
	{
		const VkSemaphoreTypeCreateInfo t_ci = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
			.semaphoreType = semaphore_type,
			.initialValue = 0,
		};

		const VkSemaphoreCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = &t_ci,
		};

		data d = {
			.type = semaphore_type,
		};

		VK_CHECK("create semaphore", vkCreateSemaphore(device, &create_info, nullptr, &d.semaphore));
		std::string s = name;

#ifdef _DEBUG
		const VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_SEMAPHORE,
			.objectHandle = reinterpret_cast<uint64_t>(d.semaphore),
			.pObjectName = name.c_str(),
		};
		VK_CHECK("set semaphore name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // _DEBUG

		return d;
	}

	void destroy(const VkSemaphore semaphore, const VkDevice device)
	{
		if (semaphore != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(device, semaphore, nullptr);
		}
	}
}

namespace vk_fence
{
	VkFence create(const VkDevice device, const VkFenceCreateFlags flags, const std::string& name)
	{
		const VkFenceCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = flags,
		};

		VkFence fence = VK_NULL_HANDLE;

		VK_CHECK("create fence", vkCreateFence(device, &create_info, nullptr, &fence));

#ifdef _DEBUG
		const VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_FENCE,
			.objectHandle = reinterpret_cast<uint64_t>(fence),
			.pObjectName = name.c_str(),
		};

		VK_CHECK("setting sampler name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // _DEBUG

		return fence;
	}

	void destroy(const VkFence fence, const VkDevice device)
	{
		if (fence != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
		{
			vkDestroyFence(device, fence, nullptr);
		}
	}
}

namespace vk_buffer
{
	VkBuffer create(const VkDevice device, const VkDeviceSize size, const VkBufferUsageFlags usage, const std::string& name)
	{
		const VkBufferCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = usage,
		};

		VkBuffer buffer = VK_NULL_HANDLE;
		VK_CHECK("create buffer", vkCreateBuffer(device, &create_info, nullptr, &buffer));

#ifdef _DEBUG
		const VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_BUFFER,
			.objectHandle = reinterpret_cast<uint64_t>(buffer),
			.pObjectName = name.c_str(),
		};

		VK_CHECK("setting buffer name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // _DEBUG

		return buffer;
	}

	void destroy(const VkBuffer buffer, const VkDevice device)
	{
		if (buffer != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, buffer, nullptr);
		}
	}
}

namespace vk_device_memory
{
	VkDeviceMemory allocate(const VkDevice device, const VkDeviceSize size, const uint32_t type_id, const std::string& name)
	{
		const VkMemoryAllocateInfo alloc_info = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = size,
			.memoryTypeIndex = type_id,
		};

		VkDeviceMemory memory = VK_NULL_HANDLE;
		VK_CHECK("allocate memory", vkAllocateMemory(device, &alloc_info, nullptr, &memory));

#ifdef _DEBUG
		VkDebugUtilsObjectNameInfoEXT name_info = {};
		name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		name_info.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY;
		name_info.objectHandle = reinterpret_cast<uint64_t>(memory);
		name_info.pObjectName = name.c_str();

		VK_CHECK("setting device memory name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // _DEBUG

		return memory;
	}

	VkDeviceMemory allocate(const VkDevice device, const VkDeviceSize size, const uint32_t type_id, const VkMemoryAllocateFlagsInfo& flags_info, const std::string& name)
	{
		const VkMemoryAllocateInfo alloc_info = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = &flags_info,
			.allocationSize = size,
			.memoryTypeIndex = type_id,
		};

		VkDeviceMemory memory = VK_NULL_HANDLE;
		VK_CHECK("allocate memory", vkAllocateMemory(device, &alloc_info, nullptr, &memory));

#ifdef _DEBUG
		const VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY,
			.objectHandle = reinterpret_cast<uint64_t>(memory),
			.pObjectName = name.c_str(),
		};

		VK_CHECK("setting device memory name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // _DEBUG

		return memory;
	}

	void free(const VkDeviceMemory memory, const VkDevice device)
	{
		if (memory != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
		{
			vkFreeMemory(device, memory, nullptr);
		}
	}
}

namespace vk_image
{
	struct data
	{
		VkImage image;
		VkDeviceMemory memory;
	};

	data create(const VkDevice device, const VkExtent3D& extent, const VkFormat format, const VkImageUsageFlags usage, const std::string& name, const bool allocate_memory = false, const VkPhysicalDeviceMemoryProperties2 mem_props = {})
	{
		data d = {};

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
		};

		VK_CHECK("create image", vkCreateImage(device, &create_info, nullptr, &d.image));

		if (allocate_memory)
		{
			const VkImageMemoryRequirementsInfo2 mem_req_info = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
				.image = d.image,
			};
			VkMemoryRequirements2 mem_reqs = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
			};
			vkGetImageMemoryRequirements2(device, &mem_req_info, &mem_reqs);
			d.memory = vk_device_memory::allocate(device, ALIGNED_SIZE(mem_reqs.memoryRequirements.size, mem_reqs.memoryRequirements.alignment), get_memory_type_id(mem_props, mem_reqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), std::string(name).append(" device memory"));

			const VkBindImageMemoryInfo bind_infos[] = {
				{
					.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
					.image = d.image,
					.memory = d.memory,
				},
			};

			VK_CHECK("bind staging image memory", vkBindImageMemory2(device, _countof(bind_infos), bind_infos));
		}

#ifdef _DEBUG
		const VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_IMAGE,
			.objectHandle = reinterpret_cast<uint64_t>(d.image),
			.pObjectName = name.c_str(),
		};

		VK_CHECK("setting image name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif //  _DEBUG

		return d;
	}

	void destroy(const data d, const VkDevice device)
	{
		if (d.image != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
		{
			vkDestroyImage(device, d.image, nullptr);
		}

		if (d.memory != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
		{
			vkFreeMemory(device, d.memory, nullptr);
		}
	}
}

namespace vk_image_view
{
	VkImageView create(const VkDevice device, const VkImage image, const VkImageViewType view_type, const VkFormat format, const VkImageAspectFlags aspect_mask, const std::string& name)
	{
		const VkImageViewCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = view_type,
			.format = format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_R,
				.g = VK_COMPONENT_SWIZZLE_G,
				.b = VK_COMPONENT_SWIZZLE_B,
				.a = VK_COMPONENT_SWIZZLE_A,
			},
			.subresourceRange = {
				.aspectMask = aspect_mask,
				.levelCount = 1,
				.layerCount = 1,
			},
		};

		VkImageView iv = VK_NULL_HANDLE;

		VK_CHECK("create image view", vkCreateImageView(device, &create_info, nullptr, &iv));

#ifdef _DEBUG
		const VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_IMAGE_VIEW,
			.objectHandle = reinterpret_cast<uint64_t>(iv),
			.pObjectName = name.c_str(),
		};

		VK_CHECK("setting image view name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif //  DEBUG

		return iv;
	}

	void destroy(const VkImageView iv, const VkDevice device)
	{
		if (iv != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
		{
			vkDestroyImageView(device, iv, nullptr);
		}
	}
};

namespace host_buffer_memory
{
	struct data
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;

		// Only valid for buffers with VK**SHADER_ADDRESS_BIT
		VkDeviceAddress addr = 0;
		void* map = nullptr;

		VkBufferUsageFlags usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
		VkDeviceSize size = 0;
	};

	data create(const VkDevice device, const VkPhysicalDeviceMemoryProperties2& mem_props, const VkDeviceSize offset, const VkBufferUsageFlags usage, const std::vector<uint8_t>& data, const std::string& name)
	{
		host_buffer_memory::data d;

		d.buffer = vk_buffer::create(device, data.size(), usage, name + " buffer");
		const VkBufferMemoryRequirementsInfo2 buff_mem_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
			.buffer = d.buffer,
		};

		VkMemoryRequirements2 mem_reqs = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
		};

		vkGetBufferMemoryRequirements2(device, &buff_mem_info, &mem_reqs);
		uint32_t mem_types = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			const VkMemoryAllocateFlagsInfo flags_info = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
				.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
			};

			d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs, mem_types), flags_info, name + " memory");
		}
		else
		{
			d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs, mem_types), name + " memory");
		}

		VK_CHECK("bind device buffer to memory", vkBindBufferMemory(device, d.buffer, d.memory, 0));

		if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			const VkBufferDeviceAddressInfo info = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
				.buffer = d.buffer,
			};

			d.addr = vkGetBufferDeviceAddress(device, &info);
			d.usage = usage;
		}

		VK_CHECK("map memory", vkMapMemory(device, d.memory, offset, data.size(), 0, &d.map));

		memcpy(d.map, data.data(), data.size());
		const VkMappedMemoryRange mem_range = {
			.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			.memory = d.memory,
			.offset = offset,
			.size = data.size(),
		};
		VK_CHECK("flush memory", vkFlushMappedMemoryRanges(device, 1, &mem_range));

		return d;
	}

	data create(const VkDevice device, const VkPhysicalDeviceMemoryProperties2& mem_props, const VkDeviceSize size, const VkBufferUsageFlags usage, const std::string& name)
	{
		host_buffer_memory::data d;

		d.buffer = vk_buffer::create(device, size, usage, name + " buffer");
		const VkBufferMemoryRequirementsInfo2 buff_mem_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
			.buffer = d.buffer,
		};

		VkMemoryRequirements2 mem_reqs = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
		};

		vkGetBufferMemoryRequirements2(device, &buff_mem_info, &mem_reqs);
		uint32_t mem_types = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			const VkMemoryAllocateFlagsInfo flags_info = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
				.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
			};

			d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs, mem_types), flags_info, name + " memory");
		}
		else
		{
			d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs, mem_types), name + " memory");
		}

		VK_CHECK("bind device buffer to memory", vkBindBufferMemory(device, d.buffer, d.memory, 0));

		if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			const VkBufferDeviceAddressInfo info = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
				.buffer = d.buffer,
			};

			d.addr = vkGetBufferDeviceAddress(device, &info);
			d.usage = usage;
		}

		VK_CHECK("map memory", vkMapMemory(device, d.memory, 0, size, 0, &d.map));

		return d;
	}

	void destroy(const data bm, const VkDevice device)
	{
		vk_buffer::destroy(bm.buffer, device);
		vk_device_memory::free(bm.memory, device);
	}
}

namespace device_buffer_memory
{
	struct data
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;

		// Only valid for buffers with VK**SHADER_DEVICE_ADDRESS_BIT
		VkDeviceAddress addr = 0;
		VkBufferUsageFlags usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
	};

	data create(const VkDevice device, const VkPhysicalDeviceMemoryProperties2& mem_props, const VkDeviceSize size, const VkBufferUsageFlags usage, const std::string& name)
	{
		device_buffer_memory::data d;
		d.buffer = vk_buffer::create(device, size, usage, name + " buffer");
		VkBufferMemoryRequirementsInfo2 buff_mem_info = {};
		buff_mem_info.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
		buff_mem_info.buffer = d.buffer;

		VkMemoryRequirements2 mem_reqs = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
		};

		vkGetBufferMemoryRequirements2(device, &buff_mem_info, &mem_reqs);
		uint32_t mem_types = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			const VkMemoryAllocateFlagsInfo flags_info = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
				.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
			};
			d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs, mem_types), flags_info, name + " memory");
		}
		else
		{
			d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs, mem_types), name + " memory");
		}

		VK_CHECK("bind device buffer to memory", vkBindBufferMemory(device, d.buffer, d.memory, 0));

		if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			const VkBufferDeviceAddressInfo info = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
				.buffer = d.buffer,
			};

			d.addr = vkGetBufferDeviceAddress(device, &info);
			d.usage = usage;
		}

		return d;
	}

	data create(const VkDevice device, const VkPhysicalDeviceMemoryProperties2& mem_props, const VkDeviceSize offset, VkBufferUsageFlags usage, const std::vector<uint8_t>& data, const VkQueue xfer_q, const VkCommandBuffer cmd_buff, const std::string& name)
	{
		host_buffer_memory::data host_buff_mem = host_buffer_memory::create(device, mem_props, offset, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, data, name + " host buffer memory");

		device_buffer_memory::data d;
		d.buffer = vk_buffer::create(device, data.size(), usage, name + " buffer");
		const VkBufferMemoryRequirementsInfo2 buff_mem_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
			.buffer = d.buffer,
		};

		VkMemoryRequirements2 mem_reqs = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
		};

		vkGetBufferMemoryRequirements2(device, &buff_mem_info, &mem_reqs);
		uint32_t mem_types = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			const VkMemoryAllocateFlagsInfo flags_info = {
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
				.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
			};

			d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs, mem_types), flags_info, name + " memory");
		}
		else
		{
			d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs, mem_types), name + " memory");
		}

		VK_CHECK("bind device buffer to memory", vkBindBufferMemory(device, d.buffer, d.memory, 0));

		if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			const VkBufferDeviceAddressInfo info = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
				.buffer = d.buffer,
			};

			d.addr = vkGetBufferDeviceAddress(device, &info);
			d.usage = usage;
		}

		std::vector<VkBufferCopy> regions(1);
		regions[0].size = data.size();

		copy_buffer_to_buffer(host_buff_mem.buffer, d.buffer, regions, cmd_buff, xfer_q);

		host_buffer_memory::destroy(host_buff_mem, device);

		return d;
	}

	void destroy(const data bm, const VkDevice device)
	{
		vk_buffer::destroy(bm.buffer, device);
		vk_device_memory::free(bm.memory, device);
	}
}
