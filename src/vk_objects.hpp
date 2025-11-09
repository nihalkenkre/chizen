#pragma once

extern "C" PFN_vkSetDebugUtilsObjectNameEXT vk_SetDebugUtilsObjectNameEXT = nullptr;

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(
	VkDevice                                    device,
	const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
	return vk_SetDebugUtilsObjectNameEXT(device, pNameInfo);
}

#define VK_CHECK(action, result)						\
	if (result != VK_SUCCESS)							\
	{															\
		std::printf("%s %d\n", action, result);	\
	}

#define SLANG_CHECK(action, result)					\
	if (result != SLANG_OK)								\
{																\
	std::printf("%s %d\n", action, result);		\
}

struct dim2d
{
	uint32_t width = 0;
	uint32_t height = 0;
};

dim2d operator+(const dim2d& lhs, const dim2d& rhs)
{
	return { lhs.width + rhs.width, lhs.height + rhs.height };
}

dim2d operator-(const dim2d& lhs, const dim2d& rhs)
{
	return { lhs.width - lhs.height, lhs.height - rhs.height };
}

void operator +=(dim2d& lhs, const dim2d& rhs)
{
	lhs.width += rhs.width;
	lhs.height += rhs.height;
}

void operator -=(dim2d& lhs, const dim2d& rhs)
{
	lhs.width -= rhs.width;
	lhs.height -= rhs.height;
}

dim2d operator*(const dim2d& lhs, const float multiplier)
{
	return { static_cast<uint32_t>(static_cast<float>(lhs.width) * multiplier), static_cast<uint32_t>(static_cast<float>(lhs.height) * multiplier) };
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
	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

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

	VK_CHECK("queue submit", vkQueueSubmit(xfer_q, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));
	VK_CHECK("queue wait idle", vkQueueWaitIdle(xfer_q));
	VK_CHECK("reset buffer copy cmd buff", vkResetCommandBuffer(cmd_buff, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
}

void copy_buffer_to_image(
	const VkBuffer src_buffer,
	const VkImage dst_image,
	const VkImageLayout dst_image_layout,
	const std::vector<VkBufferImageCopy2>& regions,
	const VkCommandBuffer cmd_buff,
	const VkQueue xfer_q)
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

	VK_CHECK("queue submit", vkQueueSubmit(xfer_q, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));
	VK_CHECK("queue wait idle", vkQueueWaitIdle(xfer_q));
	VK_CHECK("reset buffer copy cmd buff", vkResetCommandBuffer(cmd_buff, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
}

void change_image_layout(
	const VkCommandBuffer cmd_buff,
	const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask,
	const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask,
	const VkImageLayout old_layout, const VkImageLayout new_layout,
	const uint32_t src_q_fly_idx, const uint32_t dst_q_fly_idx,
	const VkImage& image)
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
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};

	const VkDependencyInfo dep_info = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &img_mem_barr,
	};

	vkCmdPipelineBarrier2(cmd_buff, &dep_info);
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

		vk_SetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));

		return instance;
	}

	void destroy(const VkInstance instance)
	{
		vkDestroyInstance(instance, nullptr);
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

struct queue_info
{
	uint32_t q_fly_idx;
	float priority;
	VkQueueFlagBits q_type;
};

namespace vk_phydev
{
	struct data
	{
		VkPhysicalDevice phy_dev = VK_NULL_HANDLE;
		uint32_t gfx_q_fly_idx = 0;
		uint32_t xfer_q_fly_idx = 0;
		uint32_t cmpt_q_fly_idx = 0;
		VkPhysicalDeviceProperties2 props = {};
		VkPhysicalDeviceMemoryProperties2 mem_props = {};
	};

	data get_phy_dev(const VkInstance instance, vk_surface::data* surface)
	{
		uint32_t phy_dev_count = 0;
		VK_CHECK("enumerate physical devices", vkEnumeratePhysicalDevices(instance, &phy_dev_count, nullptr));

		std::vector<VkPhysicalDevice> phy_devs(phy_dev_count);
		VK_CHECK("enumerate physical devices", vkEnumeratePhysicalDevices(instance, &phy_dev_count, phy_devs.data()));

		vk_phydev::data d = {
			.props = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
			},
			.mem_props = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
			},
		};

		for (auto const& phy_dev : phy_devs)
		{
			VkPhysicalDeviceProperties2 props = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
			};

			vkGetPhysicalDeviceProperties2(phy_dev, &props);

			if (props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				uint32_t q_fly_cnt = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(phy_dev, &q_fly_cnt, nullptr);

				std::vector<VkQueueFamilyProperties> q_fly_props(q_fly_cnt);
				vkGetPhysicalDeviceQueueFamilyProperties(phy_dev, &q_fly_cnt, q_fly_props.data());

				// find graphics queue
				for (uint32_t q = 0; q < q_fly_cnt; ++q)
				{
					VkBool32 is_supported = false;
					vkGetPhysicalDeviceSurfaceSupportKHR(phy_dev, q, surface->surface, &is_supported);

					if (is_supported &&
						SDL_Vulkan_GetPresentationSupport(instance, phy_dev, q) &&
						q_fly_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					{
						d.phy_dev = phy_dev;
						d.gfx_q_fly_idx = q;
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

						break;
					}
				}

				// Find transfer queue
				for (int32_t q = q_fly_cnt - 1; q >= 0; --q)
				{
					if ((q_fly_props[q].queueFlags & VK_QUEUE_TRANSFER_BIT) && !(q_fly_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(q_fly_props[q].queueFlags & VK_QUEUE_COMPUTE_BIT))
					{
						d.xfer_q_fly_idx = q;
						break;
					}
					else if ((q_fly_props[q].queueFlags & VK_QUEUE_TRANSFER_BIT) && !(q_fly_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT))
					{
						d.xfer_q_fly_idx = q;
						break;
					}
					else if (q_fly_props[q].queueFlags & VK_QUEUE_TRANSFER_BIT)
					{
						d.xfer_q_fly_idx = q;
						break;
					}
				}

				// Find compute queue
				for (int32_t q = q_fly_cnt - 1; q >= 0; --q)
				{
					if ((q_fly_props[q].queueFlags & VK_QUEUE_COMPUTE_BIT) && !(q_fly_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(q_fly_props[q].queueFlags & VK_QUEUE_TRANSFER_BIT))
					{
						d.cmpt_q_fly_idx = q;
						break;
					}
					else if ((q_fly_props[q].queueFlags & VK_QUEUE_COMPUTE_BIT) && !(q_fly_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT))
					{
						d.cmpt_q_fly_idx = q;
						break;
					}
					else if (q_fly_props[q].queueFlags & VK_QUEUE_COMPUTE_BIT)
					{
						d.cmpt_q_fly_idx = q;
						break;
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
		VkQueue cmpt_q = VK_NULL_HANDLE;
		VkQueue xfer_q = VK_NULL_HANDLE;
	};

	data create(const vk_phydev::data phy_dev_data)
	{
		std::vector<const char*> req_ext_names = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_MAINTENANCE_6_EXTENSION_NAME,
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

		VkDeviceQueueCreateInfo q_ci = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = 0,
			.queueCount = 1,
		};

		std::vector<VkDeviceQueueCreateInfo> d_q_cis{ q_ci };
		{
			auto d_q_ci_it = std::find_if(std::begin(d_q_cis), std::end(d_q_cis), [&](const VkDeviceQueueCreateInfo d_q_ci) {return d_q_ci.queueFamilyIndex == phy_dev_data.xfer_q_fly_idx; });

			if (d_q_ci_it == std::end(d_q_cis))
			{
				q_ci.queueFamilyIndex = phy_dev_data.xfer_q_fly_idx;
				q_ci.queueCount = 1;
				d_q_cis.push_back(q_ci);
			}
			else
			{
				++d_q_ci_it->queueCount;
			}

			d_q_ci_it = std::find_if(std::begin(d_q_cis), std::end(d_q_cis), [&](const VkDeviceQueueCreateInfo d_q_ci) {return d_q_ci.queueFamilyIndex == phy_dev_data.cmpt_q_fly_idx; });

			if (d_q_ci_it == std::end(d_q_cis))
			{
				q_ci.queueFamilyIndex = phy_dev_data.cmpt_q_fly_idx;
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

		VkPhysicalDeviceDynamicRenderingFeatures dyn_rend_feats = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
		};

		VkPhysicalDeviceSynchronization2Features sync2_feats = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
			.pNext = &dyn_rend_feats,
		};

		VkPhysicalDeviceVulkan12Features feats12 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
			.pNext = &sync2_feats,
			.timelineSemaphore = true,
		};

		VkPhysicalDeviceFeatures2 feats2 = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
			.pNext = &feats12,
		};

		vkGetPhysicalDeviceFeatures2(phy_dev_data.phy_dev, &feats2);

		const VkDeviceCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = &feats2,
			.queueCreateInfoCount = static_cast<uint32_t>(d_q_cis.size()),
			.pQueueCreateInfos = d_q_cis.data(),
			.enabledExtensionCount = static_cast<uint32_t>(req_ext_names.size()),
			.ppEnabledExtensionNames = req_ext_names.data(),
		};

		data d = {};

		VK_CHECK("create device", vkCreateDevice(phy_dev_data.phy_dev, &create_info, nullptr, &d.device));

		VkDeviceQueueInfo2 queue_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
		};

		for (const auto& d_q_ci : d_q_cis)
		{
			for (uint32_t q = 0; q < d_q_ci.queueCount; ++q)
			{
				if (d_q_ci.queueFamilyIndex == phy_dev_data.gfx_q_fly_idx)
				{
					queue_info.queueFamilyIndex = phy_dev_data.gfx_q_fly_idx;
					queue_info.queueIndex = q;
					vkGetDeviceQueue2(d.device, &queue_info, &d.gfx_q);
				}
				else if (d_q_ci.queueFamilyIndex == phy_dev_data.xfer_q_fly_idx)
				{
					queue_info.queueFamilyIndex = phy_dev_data.xfer_q_fly_idx;
					queue_info.queueIndex = q;
					vkGetDeviceQueue2(d.device, &queue_info, &d.xfer_q);
				}
				else if (d_q_ci.queueFamilyIndex == phy_dev_data.cmpt_q_fly_idx)
				{
					queue_info.queueFamilyIndex = phy_dev_data.cmpt_q_fly_idx;
					queue_info.queueIndex = q;
					vkGetDeviceQueue2(d.device, &queue_info, &d.cmpt_q);
				}
			}
		}

#ifdef _DEBUG
		VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_QUEUE,
			.objectHandle = reinterpret_cast<uint64_t>(d.gfx_q),
			.pObjectName = "gfx q",
		};

		VK_CHECK("set gfx_q name", vkSetDebugUtilsObjectNameEXT(d.device, &name_info));

		name_info.objectType = VK_OBJECT_TYPE_QUEUE;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.cmpt_q);
		name_info.pObjectName = "cmpt q";

		VK_CHECK("set cmpt_q name", vkSetDebugUtilsObjectNameEXT(d.device, &name_info));

		name_info.objectType = VK_OBJECT_TYPE_QUEUE;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.xfer_q);
		name_info.pObjectName = "xfer q";

		VK_CHECK("set xfer_q name", vkSetDebugUtilsObjectNameEXT(d.device, &name_info));

#endif	// _DEBUG

		return d;
	}

	void destroy(const VkDevice device)
	{
		vkDestroyDevice(device, nullptr);
	}
}

namespace vk_semaphore
{
	struct data
	{
		VkSemaphore semaphore = VK_NULL_HANDLE;
		VkSemaphoreType type = VK_SEMAPHORE_TYPE_BINARY;
		uint64_t value = 0;
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
		if (device != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(device, semaphore, nullptr);
		}
	}
}

namespace vk_command_pool
{
	struct data
	{
		VkCommandPool gfx_cmd_pool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> gfx_cmd_buffs;
	};

	data create(const VkDevice device, const uint32_t q_fly_idx, const uint32_t cmd_buffs_count, const std::string& name)
	{
		const VkCommandPoolCreateInfo cmd_pool_ci = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = q_fly_idx,
		};

		vk_command_pool::data d;

		VK_CHECK("create command pool", vkCreateCommandPool(device, &cmd_pool_ci, nullptr, &d.gfx_cmd_pool));

		const VkCommandBufferAllocateInfo allocate_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = d.gfx_cmd_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = cmd_buffs_count,
		};

		d.gfx_cmd_buffs.resize(cmd_buffs_count);

		VK_CHECK("allocate command buffer", vkAllocateCommandBuffers(device, &allocate_info, d.gfx_cmd_buffs.data()));

#ifdef _DEBUG
		std::string n = name;
		VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_COMMAND_POOL,
			.objectHandle = reinterpret_cast<uint64_t>(d.gfx_cmd_pool),
			.pObjectName = name.c_str(),
		};

		VK_CHECK("setting command pool name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		for (size_t cb = 0; cb < d.gfx_cmd_buffs.size(); ++cb)
		{
			n = name;
			name_info.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.gfx_cmd_buffs[cb]);
			name_info.pObjectName = n.append(" command buffer ").append(std::to_string(cb)).c_str();

			VK_CHECK("setting command pool name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
		}
#endif // _DEBUG

		return d;
	}

	void destroy(vk_command_pool::data gfx_cmd_pool, const VkDevice device)
	{
		if (device != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(device, gfx_cmd_pool.gfx_cmd_pool, nullptr);
		}
	}
}

namespace vk_compute_pipeline
{
	struct data
	{
		VkDescriptorPool dsp = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> dss;
		VkDescriptorSetLayout dsl;
		VkPipelineLayout lyt = VK_NULL_HANDLE;
		VkPipeline pipe = VK_NULL_HANDLE;
	};

	struct PushConstants
	{
		uint32_t dispatch_x = 32;
		uint32_t dispatch_y = 32;
		uint32_t current_time = 0;
		uint32_t curr_sample = 0;
	};

	data create(const VkDevice device, const std::string current_path, const uint8_t max_frames_in_flight, const std::string name)
	{
		data d = {};

		const std::string shader_path = std::string(current_path).append("/shaders/render.comp.glsl.spv");
		std::vector<char> shader_code(std::filesystem::file_size(shader_path));
		std::ifstream shader_file(shader_path.c_str(), std::ios::binary | std::ios::in);

		shader_file.read(shader_code.data(), shader_code.size());

		const VkShaderModuleCreateInfo mod_ci = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = shader_code.size(),
			.pCode = reinterpret_cast<const uint32_t*>(shader_code.data()),
		};

		VkShaderModule mod = VK_NULL_HANDLE;
		VK_CHECK("create shader module", vkCreateShaderModule(device, &mod_ci, nullptr, &mod));

		const VkPipelineShaderStageCreateInfo stage_ci = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = mod,
			.pName = "main",
		};

		const VkDescriptorSetLayoutBinding dsl_binds[] = {
			{
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
			},
			{
				.binding = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
			},
		};

		const VkDescriptorSetLayoutCreateInfo dsl_ci = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = std::size(dsl_binds),
			.pBindings = dsl_binds,
		};

		VK_CHECK("create dsl", vkCreateDescriptorSetLayout(device, &dsl_ci, nullptr, &d.dsl));

		const VkDescriptorPoolSize pool_sizes[] = {
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = 1,
			},
		};

		const VkDescriptorPoolCreateInfo dp_ci = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = max_frames_in_flight,
			.poolSizeCount = std::size(pool_sizes),
			.pPoolSizes = pool_sizes,
		};

		VK_CHECK("create dsp", vkCreateDescriptorPool(device, &dp_ci, nullptr, &d.dsp));

		const VkDescriptorSetAllocateInfo ds_ai = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = d.dsp,
			.descriptorSetCount = 1,
			.pSetLayouts = &d.dsl,
		};

		d.dss.resize(max_frames_in_flight);
		for (uint8_t fr = 0; fr < max_frames_in_flight; ++fr)
		{
			VK_CHECK("allocate ds", vkAllocateDescriptorSets(device, &ds_ai, &d.dss[fr]));
		}

		const VkPushConstantRange pc_rngs[] = {
			{
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.size = sizeof(PushConstants),
			},
		};

		const VkPipelineLayoutCreateInfo pip_lyt_ci = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &d.dsl,
			.pushConstantRangeCount = std::size(pc_rngs),
			.pPushConstantRanges = pc_rngs,
		};

		VK_CHECK("create pipe lyt", vkCreatePipelineLayout(device, &pip_lyt_ci, nullptr, &d.lyt));

		const VkComputePipelineCreateInfo p_cis[] = {
			{
				.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				.stage = stage_ci,
				.layout = d.lyt,
			},
		};

		VK_CHECK("create compute pipeline", vkCreateComputePipelines(device, VK_NULL_HANDLE, std::size(p_cis), p_cis, nullptr, &d.pipe));

		vkDestroyShaderModule(device, mod, nullptr);

#ifdef _DEBUG

		std::string n = name;
		VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL,
			.objectHandle = reinterpret_cast<uint64_t>(d.dsp),
			.pObjectName = n.append(" desc pool").c_str(),
		};

		VK_CHECK("setting cmpt ppln dsp name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		n = name;
		name_info.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.dsl);
		name_info.pObjectName = n.append(" desc set lyt").c_str();
		VK_CHECK("setting cmpt ppln dsl name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		for (uint8_t fr = 0; fr < max_frames_in_flight; ++fr)
		{
			n = name;
			name_info.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.dss[fr]);
			name_info.pObjectName = n.append(" desc set ").append(std::to_string(fr)).c_str();
			VK_CHECK("setting cmpt ppln ds name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
		}

		n = name;
		name_info.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.lyt);
		name_info.pObjectName = n.append(" pipe lyt").c_str();

		VK_CHECK("setting cmpt ppln lyt name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		n = name;
		name_info.objectType = VK_OBJECT_TYPE_PIPELINE;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.pipe);
		name_info.pObjectName = n.append(" pipe").c_str();

		VK_CHECK("setting cmpt ppln name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif	// _DEBUG

		return d;
	}

	void destroy(data d, const VkDevice device)
	{
		if (device != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(device, d.dsp, nullptr);
			vkDestroyDescriptorSetLayout(device, d.dsl, nullptr);
			vkDestroyPipeline(device, d.pipe, nullptr);
			vkDestroyPipelineLayout(device, d.lyt, nullptr);
		}
	}
}

namespace vk_graphics_pipeline
{
	struct data
	{
		VkDescriptorPool dsp = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> dss;
		VkDescriptorSetLayout dsl;
		VkPipelineLayout lyt = VK_NULL_HANDLE;
		VkPipeline pipe = VK_NULL_HANDLE;
	};

	struct PushConstants
	{
		float pos_offset[2];
		float zoom_level;
	};

	data create(const VkDevice device, const std::string& current_path, const VkFormat& format, const uint8_t max_frames_in_flight, const std::string& name)
	{
		data d = {};

		const std::string vert_shader_path = std::string(current_path).append("/shaders/display.vert.glsl.spv");
		std::vector<char> vert_shader_code(std::filesystem::file_size(vert_shader_path));
		std::ifstream vert_shader_file(vert_shader_path.c_str(), std::ios::binary | std::ios::in);

		vert_shader_file.read(vert_shader_code.data(), vert_shader_code.size());

		const VkShaderModuleCreateInfo vert_mod_ci = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = vert_shader_code.size(),
			.pCode = reinterpret_cast<const uint32_t*>(vert_shader_code.data()),
		};

		VkShaderModule vert_mod = VK_NULL_HANDLE;
		VK_CHECK("create vert shader module", vkCreateShaderModule(device, &vert_mod_ci, nullptr, &vert_mod));

		const std::string frag_shader_path = std::string(current_path).append("/shaders/display.frag.glsl.spv");
		std::vector<char> frag_shader_code(std::filesystem::file_size(frag_shader_path));
		std::ifstream frag_shader_file(frag_shader_path.c_str(), std::ios::binary | std::ios::in);

		frag_shader_file.read(frag_shader_code.data(), frag_shader_code.size());

		const VkShaderModuleCreateInfo frag_mod_ci = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = frag_shader_code.size(),
			.pCode = reinterpret_cast<const uint32_t*>(frag_shader_code.data()),
		};

		VkShaderModule frag_mod = VK_NULL_HANDLE;
		VK_CHECK("create frag shader module", vkCreateShaderModule(device, &frag_mod_ci, nullptr, &frag_mod));

		const VkPipelineShaderStageCreateInfo stages[] = {
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = vert_mod,
				.pName = "main",
			},
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = frag_mod,
				.pName = "main",
			},
		};

		const VkVertexInputBindingDescription vbds[] = {
			{
				.binding = 0,
				.stride = sizeof(float) * 4,
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
			},
		};

		const VkVertexInputAttributeDescription vads[] = {
			{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
			},
			{
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = sizeof(float) * 2,
			},
		};

		const VkPipelineVertexInputStateCreateInfo vis_ci = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = std::size(vbds),
			.pVertexBindingDescriptions = vbds,
			.vertexAttributeDescriptionCount = std::size(vads),
			.pVertexAttributeDescriptions = vads,
		};

		const VkViewport viewports[] = {
			{},
		};

		const VkRect2D scissors[] = {
			{},
		};

		const VkPipelineViewportStateCreateInfo vs_ci = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = std::size(viewports),
			.pViewports = viewports,
			.scissorCount = std::size(scissors),
			.pScissors = scissors,
		};

		const VkPipelineInputAssemblyStateCreateInfo ias_ci = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		};

		const VkPipelineRasterizationStateCreateInfo ras_ci = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
		};

		const VkPipelineMultisampleStateCreateInfo ms_ci = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		};

		const VkPipelineColorBlendAttachmentState cbas[] = {
			{
				.blendEnable = VK_TRUE,
				.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
				.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				.colorBlendOp = VK_BLEND_OP_ADD,
				.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
				.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
				.alphaBlendOp = VK_BLEND_OP_ADD,
				.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
			},
		};

		const VkPipelineColorBlendStateCreateInfo cbs_ci = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.attachmentCount = std::size(cbas),
			.pAttachments = cbas,
		};

		std::vector<VkDynamicState> ds = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};

		const VkPipelineDynamicStateCreateInfo ds_ci = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = static_cast<uint32_t>(ds.size()),
			.pDynamicStates = ds.data(),
		};

		const VkDescriptorSetLayoutBinding dsl_binds[] = {
			{
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			},
		};

		const VkDescriptorSetLayoutCreateInfo dsl_ci = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = std::size(dsl_binds),
			.pBindings = dsl_binds,
		};

		VK_CHECK("create dsl", vkCreateDescriptorSetLayout(device, &dsl_ci, nullptr, &d.dsl));

		const VkDescriptorPoolSize pool_sizes[] = {
			{
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
			},
		};

		const VkDescriptorPoolCreateInfo dp_ci = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = max_frames_in_flight,
			.poolSizeCount = std::size(pool_sizes),
			.pPoolSizes = pool_sizes,
		};

		VK_CHECK("create dsp", vkCreateDescriptorPool(device, &dp_ci, nullptr, &d.dsp));

		const VkDescriptorSetAllocateInfo ds_ai = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = d.dsp,
			.descriptorSetCount = 1,
			.pSetLayouts = &d.dsl,
		};

		d.dss.resize(max_frames_in_flight);
		for (uint8_t fr = 0; fr < max_frames_in_flight; ++fr)
		{
			VK_CHECK("allocate ds", vkAllocateDescriptorSets(device, &ds_ai, &d.dss[fr]));
		}

		const VkPushConstantRange pc_rngs[] = {
			{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
				.size = sizeof(PushConstants),
			},
		};

		const VkPipelineLayoutCreateInfo lyt_ci = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &d.dsl,
			.pushConstantRangeCount = std::size(pc_rngs),
			.pPushConstantRanges = pc_rngs,
		};

		VK_CHECK("create graphics pipeline layout", vkCreatePipelineLayout(device, &lyt_ci, nullptr, &d.lyt));

		const VkFormat col_attach_forms[] = {
			format,
		};

		const VkPipelineRenderingCreateInfo rend_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = std::size(col_attach_forms),
			.pColorAttachmentFormats = col_attach_forms,
		};

		const VkGraphicsPipelineCreateInfo cis[] = {
			{
				.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
				.pNext = &rend_info,
				.stageCount = std::size(stages),
				.pStages = stages,
				.pVertexInputState = &vis_ci,
				.pInputAssemblyState = &ias_ci,
				.pViewportState = &vs_ci,
				.pRasterizationState = &ras_ci,
				.pMultisampleState = &ms_ci,
				.pColorBlendState = &cbs_ci,
				.pDynamicState = &ds_ci,
				.layout = d.lyt,
			},
		};

		VK_CHECK("create graphics pipeline", vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, std::size(cis), cis, nullptr, &d.pipe));

		vkDestroyShaderModule(device, vert_mod, nullptr);
		vkDestroyShaderModule(device, frag_mod, nullptr);

#ifdef _DEBUG

		std::string n = name;
		VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL,
			.objectHandle = reinterpret_cast<uint64_t>(d.dsp),
			.pObjectName = n.append(" desc pool").c_str(),
		};
		VK_CHECK("setting gfx ppln dsp name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		n = name;
		name_info.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.dsl);
		name_info.pObjectName = n.append(" desc set lyt").c_str();
		VK_CHECK("setting gfx ppln dsl name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		for (uint8_t fr = 0; fr < max_frames_in_flight; ++fr)
		{
			n = name;
			name_info.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.dss[fr]);
			name_info.pObjectName = n.append(" desc set ").append(std::to_string(fr)).c_str();

			VK_CHECK("setting gfx ppln ds name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
		}

		n = name;
		name_info.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.lyt);
		name_info.pObjectName = n.append(" pipe lyt").c_str();

		VK_CHECK("setting gfx ppln lyt name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		n = name;
		name_info.objectType = VK_OBJECT_TYPE_PIPELINE;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.pipe);
		name_info.pObjectName = n.append(" pipe").c_str();

		VK_CHECK("setting gfx ppln name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif	// _DEBUG

		return d;
	}

	void destroy(data d, const VkDevice device)
	{
		if (device != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(device, d.dsp, nullptr);
			vkDestroyDescriptorSetLayout(device, d.dsl, nullptr);
			vkDestroyPipeline(device, d.pipe, nullptr);
			vkDestroyPipelineLayout(device, d.lyt, nullptr);
		}
	}
}

namespace vk_swapchain
{
	struct data
	{
		VkSwapchainKHR swapchain = VK_NULL_HANDLE;
		VkCommandPool gfx_cmd_pool = VK_NULL_HANDLE;
		VkCommandPool cmpt_cmd_pool = VK_NULL_HANDLE;

		std::vector<VkImage> images;
		std::vector<VkImageView> image_views;
		std::vector<VkCommandBuffer> gfx_cmd_buffs;
		std::vector<VkCommandBuffer> cmpt_cmd_buffs;
		std::vector<VkFence> rndr_fncs;
		VkSemaphore rndr_sig_sem;
		std::vector<VkSemaphore> draw_sig_sems;
		std::vector<VkSemaphore> acq_sig_sems;
		uint8_t max_frames_in_flight = 0;
		uint32_t sc_image_count = 0;
		uint8_t frame_in_flight = 0;
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
			.pQueueFamilyIndices = &phy_dev.gfx_q_fly_idx,
			.preTransform = surface.surf_caps.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = surface.present_mode,
			.oldSwapchain = old_swapchain,
		};

		vk_swapchain::data d;
		VK_CHECK("create swapchain", vkCreateSwapchainKHR(device, &create_info, nullptr, &d.swapchain));

		VK_CHECK("get swapchain images", vkGetSwapchainImagesKHR(device, d.swapchain, &d.sc_image_count, nullptr));

		d.images.resize(d.sc_image_count);
		VK_CHECK("get swapchain images", vkGetSwapchainImagesKHR(device, d.swapchain, &d.sc_image_count, d.images.data()));

		d.max_frames_in_flight = d.sc_image_count;

		d.image_views.resize(d.sc_image_count);
		d.gfx_cmd_buffs.resize(d.max_frames_in_flight);
		d.cmpt_cmd_buffs.resize(d.max_frames_in_flight);
		d.rndr_fncs.resize(d.max_frames_in_flight);
		//d.rndr_sig_sems.resize(d.max_frames_in_flight);
		d.draw_sig_sems.resize(d.max_frames_in_flight);
		d.acq_sig_sems.resize(d.max_frames_in_flight);

		VkImageViewCreateInfo image_view_create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = surface.format.format,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			},
		};

		for (uint32_t i = 0; i < d.sc_image_count; ++i)
		{
			image_view_create_info.image = d.images[i];
			VK_CHECK("create swapchain image view", vkCreateImageView(device, &image_view_create_info, nullptr, &d.image_views[i]));
		}

		const VkCommandPoolCreateInfo gfx_cmd_pool_ci = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = phy_dev.gfx_q_fly_idx,
		};

		VK_CHECK("create command pool", vkCreateCommandPool(device, &gfx_cmd_pool_ci, nullptr, &d.gfx_cmd_pool));

		const VkCommandPoolCreateInfo cmpt_cmd_pool_ci = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = phy_dev.cmpt_q_fly_idx,
		};

		VK_CHECK("create command pool", vkCreateCommandPool(device, &cmpt_cmd_pool_ci, nullptr, &d.cmpt_cmd_pool));

		const VkCommandBufferAllocateInfo gfx_cmd_buff_ai = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = d.gfx_cmd_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		const VkCommandBufferAllocateInfo cmpt_cmd_buff_ai = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = d.cmpt_cmd_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		const VkSemaphoreCreateInfo bin_sem_ci = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};

		const VkSemaphoreTypeCreateInfo sem_type_ci = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
			.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
			.initialValue = 0,
		};

		const VkSemaphoreCreateInfo tl_sem_ci = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = &sem_type_ci,
		};

		const VkFenceCreateInfo fence_ci = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};

		for (uint8_t i = 0; i < d.max_frames_in_flight; ++i)
		{
			VK_CHECK("allocate gfx command buffer", vkAllocateCommandBuffers(device, &gfx_cmd_buff_ai, &d.gfx_cmd_buffs[i]));
			VK_CHECK("allocate cmpt command buffer", vkAllocateCommandBuffers(device, &cmpt_cmd_buff_ai, &d.cmpt_cmd_buffs[i]));
			VK_CHECK("create fence", vkCreateFence(device, &fence_ci, nullptr, &d.rndr_fncs[i]));
			VK_CHECK("create acq sig semahpore", vkCreateSemaphore(device, &bin_sem_ci, nullptr, &d.acq_sig_sems[i]));
			VK_CHECK("create draw sig semahpore", vkCreateSemaphore(device, &bin_sem_ci, nullptr, &d.draw_sig_sems[i]));
		}
		VK_CHECK("create rndr sig semahpore", vkCreateSemaphore(device, &tl_sem_ci, nullptr, &d.rndr_sig_sem));

#ifdef _DEBUG
		VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR,
			.objectHandle = reinterpret_cast<uint64_t>(d.swapchain),
			.pObjectName = name.c_str(),
		};

		VK_CHECK("set swapchain name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		std::string n(name);

		name_info.objectType = VK_OBJECT_TYPE_COMMAND_POOL;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.gfx_cmd_pool);
		name_info.pObjectName = n.append(" gfx command pool").c_str();
		VK_CHECK("setting gfx swapchain cmd pool name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		n = name;
		name_info.objectType = VK_OBJECT_TYPE_COMMAND_POOL;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.cmpt_cmd_pool);
		name_info.pObjectName = n.append(" cmpt command pool").c_str();
		VK_CHECK("setting cmpt swapchain cmd pool name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		n = name;
		name_info.objectType = VK_OBJECT_TYPE_SEMAPHORE;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.rndr_sig_sem);
		name_info.pObjectName = n.append(" rndr sig sem ").c_str();// .append(std::to_string(i)).c_str();
		VK_CHECK("setting swapchain cmpt sig sem name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		for (uint32_t i = 0; i < d.sc_image_count; ++i)
		{
			n = name;
			name_info.objectType = VK_OBJECT_TYPE_IMAGE;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.images[i]);
			name_info.pObjectName = n.append(" image ").append(std::to_string(i)).c_str();
			VK_CHECK("setting swapchain image name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

			n = name;
			name_info.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.image_views[i]);
			name_info.pObjectName = n.append(" image view ").append(std::to_string(i)).c_str();
			VK_CHECK("setting swapcahin image view name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
		}

		for (uint8_t i = 0; i < d.max_frames_in_flight; ++i)
		{
			n = name;
			name_info.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.gfx_cmd_buffs[i]);
			name_info.pObjectName = n.append(" gfx cmd buff ").append(std::to_string(i)).c_str();
			VK_CHECK("setting swapchain gfx command buffer name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

			n = name;
			name_info.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.cmpt_cmd_buffs[i]);
			name_info.pObjectName = n.append(" cmpt cmd buff ").append(std::to_string(i)).c_str();
			VK_CHECK("setting swapchain cmpt command buffer name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

			n = name;
			name_info.objectType = VK_OBJECT_TYPE_FENCE;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.rndr_fncs[i]);
			name_info.pObjectName = n.append(" rndr fnc ").append(std::to_string(i)).c_str();
			VK_CHECK("setting swapchain render fence name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

			n = name;
			name_info.objectType = VK_OBJECT_TYPE_SEMAPHORE;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.draw_sig_sems[i]);
			name_info.pObjectName = n.append(" gfx sig sem ").append(std::to_string(i)).c_str();
			VK_CHECK("setting swapchain gfx sig sem name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

			n = name;
			name_info.objectType = VK_OBJECT_TYPE_SEMAPHORE;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.acq_sig_sems[i]);
			name_info.pObjectName = n.append(" acq sig sem ").append(std::to_string(i)).c_str();
			VK_CHECK("setting swapchain acq sig sem name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
		}
#endif // _DEBUG

		return d;
	}

	void destroy(vk_swapchain::data d, const VkDevice device)
	{
		if (device != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(device, d.gfx_cmd_pool, nullptr);
			vkDestroyCommandPool(device, d.cmpt_cmd_pool, nullptr);

			for (uint32_t i = 0; i < d.sc_image_count; ++i)
			{
				vkDestroyImageView(device, d.image_views[i], nullptr);
			}

			for (uint8_t i = 0; i < d.max_frames_in_flight; ++i)
			{
				vkDestroyFence(device, d.rndr_fncs[i], nullptr);
				vkDestroySemaphore(device, d.draw_sig_sems[i], nullptr);
				vkDestroySemaphore(device, d.acq_sig_sems[i], nullptr);
			}

			vkDestroySemaphore(device, d.rndr_sig_sem, nullptr);
			vkDestroySwapchainKHR(device, d.swapchain, nullptr);
		}
	}
};

namespace vk_command_buffer
{
	std::vector<VkCommandBuffer> allocate(const VkDevice device, const VkCommandPool gfx_cmd_pool, const uint32_t count, const std::string& name)
	{
		const VkCommandBufferAllocateInfo allocate_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = gfx_cmd_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = count,
		};

		std::vector<VkCommandBuffer> gfx_cmd_buffs(count);
		VK_CHECK("allocate command buffer", vkAllocateCommandBuffers(device, &allocate_info, gfx_cmd_buffs.data()));

#ifdef _DEBUG
		std::string n = name;
		for (uint32_t idx = 0; idx < count; ++idx)
		{
			n = name;
			const VkDebugUtilsObjectNameInfoEXT name_info = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER,
				.objectHandle = reinterpret_cast<uint64_t>(gfx_cmd_buffs[idx]),
				.pObjectName = n.append(" ").append(std::to_string(idx)).c_str(),
			};

			VK_CHECK("setting command buffer name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
		}
#endif // _DEBUG

		return gfx_cmd_buffs;
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
		if (device != VK_NULL_HANDLE)
		{
			vkDestroyFence(device, fence, nullptr);
		}
	}
}

namespace vk_buffer
{
	struct data
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		VmaAllocationInfo alloc_info = {};
	};

	data create(const VkDevice device, const VmaAllocator& allocator, const VkDeviceSize size, const VkBufferUsageFlags usage, const VmaAllocationCreateFlags vma_alloc_create_flags, const VmaMemoryUsage vma_mem_usage, const std::string& name)
	{
		data d = {};

		const VkBufferCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = usage,
		};

		const VmaAllocationCreateInfo alloc_ci = {
			.flags = vma_alloc_create_flags,
			.usage = vma_mem_usage,
		};

		VK_CHECK(std::string("create buffer").append(name).c_str(), vmaCreateBuffer(allocator, &create_info, &alloc_ci, &d.buffer, &d.allocation, &d.alloc_info));

#ifdef _DEBUG
		const VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_BUFFER,
			.objectHandle = reinterpret_cast<uint64_t>(d.buffer),
			.pObjectName = name.c_str(),
		};

		VK_CHECK("setting buffer name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // _DEBUG

		return d;
	}

	void destroy(data d, const VmaAllocator allocator, const VkDevice device)
	{
		if (device != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(allocator, d.buffer, d.allocation);
		}
	}
}

namespace vk_image
{
	struct data
	{
		VkImage image = VK_NULL_HANDLE;
		VkImageView image_view = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;
		VkImageLayout image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkDescriptorImageInfo desc_img_info = {};
		VkDeviceSize desc_offset = 0;
		VmaAllocation alloc = VK_NULL_HANDLE;
		VmaAllocationInfo alloc_info = {};
		dim2d dims = {};
	};

	data create(
		const VkDevice device,
		const VkExtent3D& extent,
		const VkFormat format,
		const VkImageUsageFlags usage,
		const VmaAllocator allocator,
		const uint32_t id,
		const std::string& name)
	{
		data d = {
			.dims = {
				.width = extent.width,
				.height = extent.height,
			},
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
		};

		const VmaAllocationCreateInfo alloc_ci = {
			.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		};

		VK_CHECK("create image", vmaCreateImage(allocator, &create_info, &alloc_ci, &d.image, &d.alloc, &d.alloc_info));

		const VkImageViewCreateInfo iv_ci = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = d.image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			},
		};

		VK_CHECK(std::string("create image view").append(name).c_str(), vkCreateImageView(device, &iv_ci, nullptr, &d.image_view));

		const VkSamplerCreateInfo s_ci = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		};

		VK_CHECK(std::string("create sampler ").append(name).c_str(), vkCreateSampler(device, &s_ci, nullptr, &d.sampler));

		d.desc_img_info = {
			.sampler = d.sampler,
			.imageView = d.image_view,
		};

#ifdef _DEBUG
		std::string n(name);

		VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_IMAGE,
			.objectHandle = reinterpret_cast<uint64_t>(d.image),
			.pObjectName = n.append(" ").append(std::to_string(id)).c_str(),
		};

		VK_CHECK(std::string("setting image name ").append(name).c_str(), vkSetDebugUtilsObjectNameEXT(device, &name_info));

		n = name;
		name_info.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.image_view);
		name_info.pObjectName = n.append(" ").append(std::to_string(id)).append(" image view").c_str();

		VK_CHECK(std::string("setting image view name ").append(name).c_str(), vkSetDebugUtilsObjectNameEXT(device, &name_info));

		n = name;
		name_info.objectType = VK_OBJECT_TYPE_SAMPLER;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.sampler);
		name_info.pObjectName = n.append(" ").append(std::to_string(id)).append(" sampler").c_str();

		VK_CHECK(std::string("setting sample name ").append(name).c_str(), vkSetDebugUtilsObjectNameEXT(device, &name_info));

#endif //  _DEBUG

		return d;
	}

	void destroy(const data d, const VmaAllocator allocator, const VkDevice device)
	{
		if (device != VK_NULL_HANDLE)
		{
			vkDestroySampler(device, d.sampler, nullptr);
			vkDestroyImageView(device, d.image_view, nullptr);
			vmaDestroyImage(allocator, d.image, d.alloc);
		}
	}
}
