#pragma once

extern "C" PFN_vkSetDebugUtilsObjectNameEXT vk_SetDebugUtilsObjectNameEXT = nullptr;
extern "C" PFN_vkGetRayTracingShaderGroupHandlesKHR vk_GetRayTracingShaderGroupHandlesKHR = nullptr;
extern "C" PFN_vkCreateRayTracingPipelinesKHR vk_CreateRayTracingPipelinesKHR = nullptr;

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(
	VkDevice                                    device,
	const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
	return vk_SetDebugUtilsObjectNameEXT(device, pNameInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void* pData)
{
	return vk_GetRayTracingShaderGroupHandlesKHR(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	return vk_CreateRayTracingPipelinesKHR(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
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

void copy_buffer_to_buffer(const VkCommandBuffer xfer_cmd_buff, const VkQueue xfer_q, const VkBuffer src_buffer, const VkBuffer dst_buffer, const VkDeviceSize size)
{
	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VK_CHECK("begin xfer cmd buff", vkBeginCommandBuffer(xfer_cmd_buff, &begin_info));

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

	vkCmdCopyBuffer2(xfer_cmd_buff, &copy_buff_info);
	VK_CHECK("end xfer cmd buff", vkEndCommandBuffer(xfer_cmd_buff));

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = xfer_cmd_buff,
		},
	};

	const VkSubmitInfo2 submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = std::size(cmd_buff_infos),
			.pCommandBufferInfos = cmd_buff_infos,
		},
	};

	VK_CHECK("submit geom buffer xfer cmd", vkQueueSubmit2(xfer_q, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));
	VK_CHECK("wait xfer cmd buff", vkQueueWaitIdle(xfer_q));
}

void insert_memory_barrier(
	const VkCommandBuffer cmd_buff,
	const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask,
	const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask
)
{
	VkMemoryBarrier2 mem_bar = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = src_stage_mask,
		.srcAccessMask = src_access_mask,
		.dstStageMask = dst_stage_mask,
		.dstAccessMask = dst_access_mask,
	};

	const VkDependencyInfo dep_info = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &mem_bar,
	};

	vkCmdPipelineBarrier2(cmd_buff, &dep_info);
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
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR rt_props = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
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
				.pNext = &d.rt_props,
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
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
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

		vk_GetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(d.device, "vkGetRayTracingShaderGroupHandlesKHR"));
		vk_CreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(d.device, "vkCreateRayTracingPipelinesKHR"));

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
		std::string n = name;
		VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_COMMAND_POOL,
			.objectHandle = reinterpret_cast<uint64_t>(d.cmd_pool),
			.pObjectName = name.c_str(),
		};

		VK_CHECK("setting command pool name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		for (size_t cb = 0; cb < d.cmd_buffs.size(); ++cb)
		{
			n = name;
			name_info.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.cmd_buffs[cb]);
			name_info.pObjectName = n.append(" command buffer ").append(std::to_string(cb)).c_str();

			VK_CHECK("setting command pool name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
		}
#endif // _DEBUG

		return d;
	}

	void destroy(vk_command_pool::data cmd_pool, const VkDevice device)
	{
		if (device != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(device, cmd_pool.cmd_pool, nullptr);
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
		uint32_t curr_sample = 0;
	};

	data create(const VkDevice device, const std::string current_path, const uint8_t max_frames_in_flight, const std::string name)
	{
		data d = {};

		Slang::ComPtr<slang::IGlobalSession> slang_global_session;
		SLANG_CHECK("create global session", slang::createGlobalSession(slang_global_session.writeRef()));

		slang::TargetDesc target_desc = {
			.format = SLANG_SPIRV,
			.profile = slang_global_session->findProfile("spirv_1_6"),
		};
		slang::SessionDesc session_desc = {
			.targets = &target_desc,
			.targetCount = 1,
		};

		Slang::ComPtr<slang::ISession> compile_session;
		SLANG_CHECK("create compile session", slang_global_session->createSession(session_desc, compile_session.writeRef()));

		const std::string slang_shader_path = std::string(current_path).append("/shaders/slang/render.slang");

		Slang::ComPtr<slang::IBlob> diagnostic_blob;
		slang::IModule* slang_module = compile_session->loadModule(slang_shader_path.c_str(), diagnostic_blob.writeRef());

		if (diagnostic_blob != nullptr)
		{
			std::println("{}", reinterpret_cast<const char*>(diagnostic_blob->getBufferPointer()));
		}

		Slang::ComPtr<slang::IEntryPoint> entry_point;
		slang_module->findEntryPointByName("compute_main", entry_point.writeRef());

		std::array<slang::IComponentType*, 2> component_types = {
			slang_module, entry_point
		};

		Slang::ComPtr<slang::IComponentType> composed_program;
		diagnostic_blob.setNull();
		SLANG_CHECK("create program", compile_session->createCompositeComponentType(component_types.data(), component_types.size(), composed_program.writeRef(), diagnostic_blob.writeRef()));

		Slang::ComPtr<slang::IComponentType> linked_program;
		diagnostic_blob.setNull();
		SLANG_CHECK("link program", composed_program->link(linked_program.writeRef(), diagnostic_blob.writeRef()));

		Slang::ComPtr<slang::IBlob> spirv_code;
		diagnostic_blob.setNull();
		SLANG_CHECK("get spirv code", composed_program->getEntryPointCode(0, 0, spirv_code.writeRef(), diagnostic_blob.writeRef()));

		const VkShaderModuleCreateInfo mod_ci = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = spirv_code->getBufferSize(),
			.pCode = reinterpret_cast<const uint32_t*>(spirv_code->getBufferPointer()),
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
			{
				.binding = 2,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
			}
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

		Slang::ComPtr<slang::IGlobalSession> slang_global_session;
		SLANG_CHECK("create global session", slang::createGlobalSession(slang_global_session.writeRef()));

		slang::TargetDesc target_desc = {
			.format = SLANG_SPIRV,
			.profile = slang_global_session->findProfile("spirv_1_6"),
		};
		slang::SessionDesc session_desc = {
			.targets = &target_desc,
			.targetCount = 1,
		};

		Slang::ComPtr<slang::ISession> compile_session;
		SLANG_CHECK("create compile session", slang_global_session->createSession(session_desc, compile_session.writeRef()));

		const std::string slang_shader_path = std::string(current_path).append("/shaders/slang/display.slang");

		Slang::ComPtr<slang::IBlob> diagnostic_blob;
		slang::IModule* slang_module = compile_session->loadModule(slang_shader_path.c_str(), diagnostic_blob.writeRef());

		if (diagnostic_blob != nullptr)
		{
			std::println("{}", reinterpret_cast<const char*>(diagnostic_blob->getBufferPointer()));
		}

		Slang::ComPtr<slang::IEntryPoint> vert_entry_point;
		slang_module->findEntryPointByName("vertex_main", vert_entry_point.writeRef());

		std::array<slang::IComponentType*, 2> vert_component_types = { slang_module, vert_entry_point };
		Slang::ComPtr<slang::IComponentType> vert_composed_program;
		diagnostic_blob.setNull();
		SLANG_CHECK("create program", compile_session->createCompositeComponentType(vert_component_types.data(), vert_component_types.size(), vert_composed_program.writeRef(), diagnostic_blob.writeRef()));

		Slang::ComPtr<slang::IComponentType> vert_linked_program;
		diagnostic_blob.setNull();
		SLANG_CHECK("link program", vert_composed_program->link(vert_linked_program.writeRef(), diagnostic_blob.writeRef()));

		Slang::ComPtr<slang::IBlob> vert_spirv_code;
		diagnostic_blob.setNull();
		SLANG_CHECK("get spirv code", vert_composed_program->getEntryPointCode(0, 0, vert_spirv_code.writeRef(), diagnostic_blob.writeRef()));

		const VkShaderModuleCreateInfo vert_mod_ci = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = vert_spirv_code->getBufferSize(),
			.pCode = reinterpret_cast<const uint32_t*>(vert_spirv_code->getBufferPointer()),
		};

		VkShaderModule vert_mod = VK_NULL_HANDLE;
		VK_CHECK("create vert shader module", vkCreateShaderModule(device, &vert_mod_ci, nullptr, &vert_mod));

		Slang::ComPtr<slang::IEntryPoint> frag_entry_point;
		slang_module->findEntryPointByName("fragment_main", frag_entry_point.writeRef());

		std::array<slang::IComponentType*, 2> frag_component_types = { slang_module, frag_entry_point };
		Slang::ComPtr<slang::IComponentType> frag_composed_program;
		diagnostic_blob.setNull();
		SLANG_CHECK("create program", compile_session->createCompositeComponentType(frag_component_types.data(), frag_component_types.size(), frag_composed_program.writeRef(), diagnostic_blob.writeRef()));

		Slang::ComPtr<slang::IComponentType> frag_linked_program;
		diagnostic_blob.setNull();
		SLANG_CHECK("link program", frag_composed_program->link(frag_linked_program.writeRef(), diagnostic_blob.writeRef()));

		Slang::ComPtr<slang::IBlob> frag_spirv_code;
		diagnostic_blob.setNull();
		SLANG_CHECK("get spirv code", frag_composed_program->getEntryPointCode(0, 0, frag_spirv_code.writeRef(), diagnostic_blob.writeRef()));

		const VkShaderModuleCreateInfo frag_mod_ci = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = frag_spirv_code->getBufferSize(),
			.pCode = reinterpret_cast<const uint32_t*>(frag_spirv_code->getBufferPointer()),
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
		uint32_t dims[2];
	};

	data create(
		const VkDevice device,
		const VkExtent3D& extent,
		const VkFormat format,
		const VkImageUsageFlags usage,
		const VmaAllocator allocator,
		const uint32_t id,
		const std::string& name,
		const std::vector<uint32_t>& q_fly_idxs = {})
	{
		data d = {
			.dims = {
				extent.width,
				extent.height,
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
			.sharingMode = VK_SHARING_MODE_CONCURRENT,
			.queueFamilyIndexCount = static_cast<uint32_t>(q_fly_idxs.size()),
			.pQueueFamilyIndices = q_fly_idxs.data(),
		};

		const VmaAllocationCreateInfo alloc_ci = {
			.flags = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
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
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
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

	void destroy(const data d, const VmaAllocator& allocator, const VkDevice device)
	{
		if (device != VK_NULL_HANDLE)
		{
			vkDestroySampler(device, d.sampler, nullptr);
			vkDestroyImageView(device, d.image_view, nullptr);
			vmaDestroyImage(allocator, d.image, d.alloc);
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
		VkDescriptorBufferInfo desc_info = {};
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

		d.desc_info = {
			.buffer = d.buffer,
			.range = VK_WHOLE_SIZE,
		};

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

namespace cmpt_swapchain
{
	struct data
	{
		VkCommandPool cmd_pool = VK_NULL_HANDLE;

		std::vector<VkCommandBuffer> cmd_buffs;
		std::vector<VkSemaphore> frame_sems;
		std::vector<uint64_t> frame_sem_vals;

		uint8_t frame_in_flight = 0;
		uint8_t max_frames_in_flight = 5;

		vk_compute_pipeline::data ppln_data = {};
		vk_image::data accum_target = {};
		vk_image::data final_render = {};
		vk_buffer::data rand_states = {};
	};

	void initialize_resources(cmpt_swapchain::data& d, const VkDevice device, const VmaAllocator allocator, const VkExtent3D& extent, const VkCommandBuffer xfer_cmd_buff, const VkQueue xfer_q)
	{
		const VkCommandBufferBeginInfo xfer_cmd_buff_bi = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		};

		VK_CHECK("begin xfer cmd buff", vkBeginCommandBuffer(xfer_cmd_buff, &xfer_cmd_buff_bi));

		change_image_layout(xfer_cmd_buff,
			VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
			VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			d.accum_target.image
		);

		change_image_layout(xfer_cmd_buff,
			VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
			VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			d.final_render.image
		);

		vk_buffer::data staging_buffer = vk_buffer::create(device, allocator, d.rand_states.alloc_info.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, "staging rand states buffer");

		for (uint32_t st = 0; st < 4 * extent.width * extent.height;)
		{
			uint32_t rand_val = rand();
			while (rand_val < 128)
				rand_val = rand();

			((uint32_t*)staging_buffer.alloc_info.pMappedData)[st++] = rand_val;
		}

		const VkBufferCopy2 regions[] = {
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
				.size = d.rand_states.alloc_info.size,
			},
		};

		const VkCopyBufferInfo2 copy_buff_info = {
			.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
			.srcBuffer = staging_buffer.buffer,
			.dstBuffer = d.rand_states.buffer,
			.regionCount = std::size(regions),
			.pRegions = regions,
		};

		vkCmdCopyBuffer2(xfer_cmd_buff, &copy_buff_info);

		VK_CHECK("end xfer cmd buff", vkEndCommandBuffer(xfer_cmd_buff));

		const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
				.commandBuffer = xfer_cmd_buff,
			}
		};

		const VkSubmitInfo2 submit_infos[] = {
			{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
				.commandBufferInfoCount = std::size(cmd_buff_infos),
				.pCommandBufferInfos = cmd_buff_infos,
			},
		};

		VK_CHECK("submit xfer cmd buff", vkQueueSubmit2(xfer_q, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));
		VK_CHECK("wait for device", vkDeviceWaitIdle(device));
		vk_buffer::destroy(staging_buffer, allocator, device);
	}

	data create(const VkDevice device, const VkExtent3D& extent, const VmaAllocator& allocator, const std::string& current_path, const uint32_t q_fly_idx, const std::vector<uint32_t>& q_fly_idxs, const VkCommandBuffer xfer_cmd_buff, const VkQueue xfer_q, const std::string& name)
	{
		data d = {};

		d.cmd_buffs.resize(d.max_frames_in_flight);
		d.frame_sems.resize(d.max_frames_in_flight);
		d.frame_sem_vals.resize(d.max_frames_in_flight, 1);

		const VkCommandPoolCreateInfo cmd_pool_ci = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = q_fly_idx,
		};

		VK_CHECK("create command pool", vkCreateCommandPool(device, &cmd_pool_ci, nullptr, &d.cmd_pool));

		const VkCommandBufferAllocateInfo cmd_buff_ai = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = d.cmd_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
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

		for (uint8_t i = 0; i < d.max_frames_in_flight; ++i)
		{
			VK_CHECK("allocate cmpt command buffer", vkAllocateCommandBuffers(device, &cmd_buff_ai, &d.cmd_buffs[i]));
			VK_CHECK("create cmpt frame semahpore", vkCreateSemaphore(device, &tl_sem_ci, nullptr, &d.frame_sems[i]));

			const VkSemaphoreSignalInfo signal_info = {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
				.semaphore = d.frame_sems[i],
				.value = 1,
			};

			VK_CHECK("signal cmpt frame sem", vkSignalSemaphore(device, &signal_info));
		}

		d.ppln_data = vk_compute_pipeline::create(device, current_path, d.max_frames_in_flight, "cmpt ppln");
		d.accum_target = vk_image::create(device, extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, allocator, 0, "accum target", q_fly_idxs);
		d.final_render = vk_image::create(device, extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, allocator, 0, "final target", q_fly_idxs);
		d.rand_states = vk_buffer::create(device, allocator, sizeof(uint32_t) * 4 * extent.width * extent.height, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, "rand states");

		initialize_resources(d, device, allocator, extent, xfer_cmd_buff, xfer_q);

#ifdef _DEBUG
		std::string n(name);
		VkDebugUtilsObjectNameInfoEXT name_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = VK_OBJECT_TYPE_COMMAND_POOL,
			.objectHandle = reinterpret_cast<uint64_t>(d.cmd_pool),
			.pObjectName = n.append(" cmd pool").c_str(),
		};

		VK_CHECK("set cmpt cmd pool name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		for (uint8_t i = 0; i < d.max_frames_in_flight; ++i)
		{
			n = name;
			name_info.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.cmd_buffs[i]);
			name_info.pObjectName = n.append(" cmpt cmd buff ").append(std::to_string(i)).c_str();
			VK_CHECK("setting swapchain cmpt command buffer name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

			n = name;
			name_info.objectType = VK_OBJECT_TYPE_SEMAPHORE;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.frame_sems[i]);
			name_info.pObjectName = n.append(" cmpt frame sem ").c_str();
			VK_CHECK("setting swapchain cmpt sig sem name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
		}

#endif	// _DEBUG

		return d;
	}

	void destroy(data d, const VmaAllocator& allocator, const VkDevice device)
	{
		if (device != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(device, d.cmd_pool, nullptr);

			for (uint8_t i = 0; i < d.max_frames_in_flight; ++i)
			{
				vkDestroySemaphore(device, d.frame_sems[i], nullptr);
			}

			vk_image::destroy(d.accum_target, allocator, device);
			vk_image::destroy(d.final_render, allocator, device);
			vk_buffer::destroy(d.rand_states, allocator, device);

			vk_compute_pipeline::destroy(d.ppln_data, device);
		}
	}

	void recreate_render_targets(data& d, const VkDevice device, const VkExtent3D& extent, const VmaAllocator& allocator, const std::vector<uint32_t>& q_fly_idxs, const VkCommandBuffer xfer_cmd_buff, const VkQueue xfer_q)
	{
		vk_image::destroy(d.accum_target, allocator, device);
		d.accum_target = vk_image::create(device, extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, allocator, 0, "accum target", q_fly_idxs);

		vk_image::destroy(d.final_render, allocator, device);
		d.final_render = vk_image::create(device, extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, allocator, 0, "final target", q_fly_idxs);

		initialize_resources(d, device, allocator, extent, xfer_cmd_buff, xfer_q);

		vk_buffer::destroy(d.rand_states, allocator, device);
		d.rand_states = vk_buffer::create(device, allocator, sizeof(uint32_t) * 4 * extent.width * extent.height, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, "rand states");
	}
}

namespace vk_swapchain
{
	struct data
	{
		VkSwapchainKHR swapchain = VK_NULL_HANDLE;
		VkCommandPool gfx_cmd_pool = VK_NULL_HANDLE;

		std::vector<VkImage> images;
		std::vector<VkImageView> image_views;
		std::vector<VkCommandBuffer> gfx_cmd_buffs;
		std::vector<VkSemaphore> gfx_frame_sems;
		std::vector<uint64_t> gfx_frame_sem_vals;
		std::vector<VkSemaphore> present_wait_sems;
		std::vector<VkSemaphore> acq_sig_sems;

		uint8_t max_frames_in_flight = 0;
		uint32_t sc_image_count = 0;
		uint8_t gfx_frame_in_flight = 0;
	};

	data create(const VkDevice device, const vk_surface::data& surface, const uint32_t gfx_q_fly_idx, const VkSwapchainKHR old_swapchain, const std::string& name)
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
			.pQueueFamilyIndices = &gfx_q_fly_idx,
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
		d.present_wait_sems.resize(d.max_frames_in_flight);
		d.acq_sig_sems.resize(d.max_frames_in_flight);
		d.gfx_frame_sems.resize(d.max_frames_in_flight);
		d.gfx_frame_sem_vals.resize(d.max_frames_in_flight, 1);

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
			.queueFamilyIndex = gfx_q_fly_idx,
		};

		VK_CHECK("create command pool", vkCreateCommandPool(device, &gfx_cmd_pool_ci, nullptr, &d.gfx_cmd_pool));

		const VkCommandBufferAllocateInfo gfx_cmd_buff_ai = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = d.gfx_cmd_pool,
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

		for (uint8_t i = 0; i < d.max_frames_in_flight; ++i)
		{
			VK_CHECK("allocate gfx command buffer", vkAllocateCommandBuffers(device, &gfx_cmd_buff_ai, &d.gfx_cmd_buffs[i]));
			VK_CHECK("create acq sig semahpore", vkCreateSemaphore(device, &bin_sem_ci, nullptr, &d.acq_sig_sems[i]));
			VK_CHECK("create present wait semahpore", vkCreateSemaphore(device, &bin_sem_ci, nullptr, &d.present_wait_sems[i]));

			VK_CHECK("create gfx frame semahpore", vkCreateSemaphore(device, &tl_sem_ci, nullptr, &d.gfx_frame_sems[i]));

			const VkSemaphoreSignalInfo signal_info = {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
				.semaphore = d.gfx_frame_sems[i],
				.value = 1,
			};

			VK_CHECK("signal gfx frame sem", vkSignalSemaphore(device, &signal_info));
		}
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
			name_info.objectType = VK_OBJECT_TYPE_SEMAPHORE;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.gfx_frame_sems[i]);
			name_info.pObjectName = n.append(" gfx frame sem ").c_str();
			VK_CHECK("setting swapchain cmpt sig sem name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

			n = name;
			name_info.objectType = VK_OBJECT_TYPE_SEMAPHORE;
			name_info.objectHandle = reinterpret_cast<uint64_t>(d.present_wait_sems[i]);
			name_info.pObjectName = n.append(" present wait sem ").append(std::to_string(i)).c_str();
			VK_CHECK("setting swapchain present wait sem name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

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

			for (uint32_t i = 0; i < d.sc_image_count; ++i)
			{
				vkDestroyImageView(device, d.image_views[i], nullptr);
			}

			for (uint8_t i = 0; i < d.max_frames_in_flight; ++i)
			{
				vkDestroySemaphore(device, d.present_wait_sems[i], nullptr);
				vkDestroySemaphore(device, d.acq_sig_sems[i], nullptr);
				vkDestroySemaphore(device, d.gfx_frame_sems[i], nullptr);
			}

			vkDestroySwapchainKHR(device, d.swapchain, nullptr);
		}
	}

	data recreate_swapchain(const vk_swapchain::data data, const VkDevice device, const vk_surface::data surf_data, const uint32_t gfx_q_fly_idx, const std::string& name)
	{
		vk_swapchain::destroy(data, device);
		return vk_swapchain::create(device, surf_data, gfx_q_fly_idx, VK_NULL_HANDLE, "swapchain");
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


struct ray_tracing_pipeline
{
	struct PushContanst
	{
		uint32_t current_sample;
	};

	VkPipeline pipeline = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> desc_set_layouts;
	vk_buffer::data rg_sbt = {};
};

ray_tracing_pipeline ray_tracing_pipeline_create(const VkDevice device, const VmaAllocator allocator, const std::string& current_path, const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& ray_tracing_props, const std::string& name)
{
	ray_tracing_pipeline rt_pipeline = {};
	rt_pipeline.desc_set_layouts.resize(1);

	Slang::ComPtr<slang::IGlobalSession> slang_global_session;
	SLANG_CHECK("create global session", slang::createGlobalSession(slang_global_session.writeRef()));

	slang::TargetDesc target_desc = {
		.format = SLANG_SPIRV,
		.profile = slang_global_session->findProfile("spirv_1_6"),
	};
	slang::SessionDesc session_desc = {
		.targets = &target_desc,
		.targetCount = 1,
	};

	Slang::ComPtr<slang::ISession> compile_session;
	SLANG_CHECK("create compile session", slang_global_session->createSession(session_desc, compile_session.writeRef()));

	const std::string slang_shader_path = std::string(current_path).append("/shaders/slang/raytrace.slang");

	Slang::ComPtr<slang::IBlob> diagnostic_blob;
	slang::IModule* slang_module = compile_session->loadModule(slang_shader_path.c_str(), diagnostic_blob.writeRef());

	if (diagnostic_blob != nullptr)
	{
		std::println("{}", reinterpret_cast<const char*>(diagnostic_blob->getBufferPointer()));
	}

	Slang::ComPtr<slang::IEntryPoint> rg_entry_point;
	slang_module->findEntryPointByName("raygen", rg_entry_point.writeRef());

	std::array<slang::IComponentType*, 2> rg_component_types = {
		slang_module, rg_entry_point
	};

	Slang::ComPtr<slang::IComponentType> rg_composed_program;
	diagnostic_blob.setNull();
	SLANG_CHECK("create program", compile_session->createCompositeComponentType(rg_component_types.data(), rg_component_types.size(), rg_composed_program.writeRef(), diagnostic_blob.writeRef()));

	Slang::ComPtr<slang::IComponentType> linked_program;
	diagnostic_blob.setNull();
	SLANG_CHECK("link program", rg_composed_program->link(linked_program.writeRef(), diagnostic_blob.writeRef()));

	Slang::ComPtr<slang::IBlob> rg_spirv_code;
	diagnostic_blob.setNull();
	SLANG_CHECK("get spirv code", rg_composed_program->getEntryPointCode(0, 0, rg_spirv_code.writeRef(), diagnostic_blob.writeRef()));

	const VkShaderModuleCreateInfo rg_mod_ci = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = rg_spirv_code->getBufferSize(),
		.pCode = reinterpret_cast<const uint32_t*>(rg_spirv_code->getBufferPointer()),
	};

	VkShaderModule rg_mod = VK_NULL_HANDLE;
	VK_CHECK("create shader module", vkCreateShaderModule(device, &rg_mod_ci, nullptr, &rg_mod));

	const VkDescriptorSetLayoutBinding bindings[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		},
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		},
	};

	const VkDescriptorSetLayoutCreateInfo dsl_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = std::size(bindings),
		.pBindings = bindings,
	};

	VK_CHECK("create desc set layout", vkCreateDescriptorSetLayout(device, &dsl_ci, nullptr, &rt_pipeline.desc_set_layouts[0]));

	const VkPushConstantRange pc_ranges[] = {
		{
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			.size = sizeof(ray_tracing_pipeline::PushContanst),
		},
	};

	const VkPipelineLayoutCreateInfo pl_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = static_cast<uint32_t>(rt_pipeline.desc_set_layouts.size()),
		.pSetLayouts = rt_pipeline.desc_set_layouts.data(),
		.pushConstantRangeCount = std::size(pc_ranges),
		.pPushConstantRanges = pc_ranges,
	};

	VK_CHECK("create pipeline layout", vkCreatePipelineLayout(device, &pl_ci, nullptr, &rt_pipeline.pipeline_layout));

	const VkPipelineShaderStageCreateInfo stage_cis[] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			.module = rg_mod,
			.pName = "main",
		},
	};

	VkRayTracingShaderGroupCreateInfoKHR shader_groups[] = {
		{
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
			.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
			.generalShader = 0,
			.closestHitShader = VK_SHADER_UNUSED_KHR,
			.anyHitShader = VK_SHADER_UNUSED_KHR,
			.intersectionShader = VK_SHADER_UNUSED_KHR,
		},
	};

	const VkRayTracingPipelineCreateInfoKHR create_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
			.stageCount = std::size(stage_cis),
			.pStages = stage_cis,
			.groupCount = std::size(shader_groups),
			.pGroups = shader_groups,
			.maxPipelineRayRecursionDepth = 1,
			.layout = rt_pipeline.pipeline_layout,
		},
	};
	VK_CHECK("create rt pipeline", vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, std::size(create_infos), create_infos, nullptr, &rt_pipeline.pipeline));

	const uint32_t sbt_size = ray_tracing_props.shaderGroupHandleSize;

	std::vector<uint8_t> shader_handle_storage(sbt_size);
	VK_CHECK("get rt shader handles", vkGetRayTracingShaderGroupHandlesKHR(device, rt_pipeline.pipeline, 0, 1, sbt_size, shader_handle_storage.data()));

	rt_pipeline.rg_sbt = vk_buffer::create(
		device, allocator, ray_tracing_props.shaderGroupHandleSize,
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST, "rb sbt");

	memcpy(rt_pipeline.rg_sbt.alloc_info.pMappedData, shader_handle_storage.data(), ray_tracing_props.shaderGroupHandleSize);

	vkDestroyShaderModule(device, rg_mod, nullptr);

	return rt_pipeline;
}

void ray_tracing_pipeline_destroy(ray_tracing_pipeline rt_pipeline, const VmaAllocator allocator, const VkDevice device)
{
	if (device != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(device, rt_pipeline.pipeline, nullptr);
		vkDestroyPipelineLayout(device, rt_pipeline.pipeline_layout, nullptr);

		for (auto& dsl : rt_pipeline.desc_set_layouts)
			vkDestroyDescriptorSetLayout(device, dsl, nullptr);

		vk_buffer::destroy(rt_pipeline.rg_sbt, allocator, device);
	}
}

struct ray_tracer
{
	VkCommandPool cmd_pool = VK_NULL_HANDLE;

	std::vector<VkCommandBuffer> cmd_buffs;
	std::vector<VkSemaphore> frame_sems;
	std::vector<uint64_t> frame_sem_vals;

	uint8_t frame_in_flight = 0;
	uint8_t max_frames_in_flight = 5;

	vk_image::data accum_target = {};
	vk_image::data final_render = {};
	vk_buffer::data rand_states = {};
};

void ray_tracer_initialize_resources(ray_tracer& rt_pipeline, const VkDevice device, const VmaAllocator allocator, const VkExtent3D& extent, const VkCommandBuffer xfer_cmd_buff, const VkQueue xfer_q)
{
	const VkCommandBufferBeginInfo xfer_cmd_buff_bi = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};

	VK_CHECK("begin xfer cmd buff", vkBeginCommandBuffer(xfer_cmd_buff, &xfer_cmd_buff_bi));

	change_image_layout(xfer_cmd_buff,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		rt_pipeline.accum_target.image
	);

	change_image_layout(xfer_cmd_buff,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		rt_pipeline.final_render.image
	);

	vk_buffer::data staging_buffer = vk_buffer::create(device, allocator, rt_pipeline.rand_states.alloc_info.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, "staging rand states buffer");

	for (uint32_t st = 0; st < 4 * extent.width * extent.height;)
	{
		uint32_t rand_val = rand();
		while (rand_val < 128)
			rand_val = rand();

		(reinterpret_cast<uint32_t*>(staging_buffer.alloc_info.pMappedData))[st++] = rand_val;
	}

	const VkBufferCopy2 regions[] = {
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
			.size = rt_pipeline.rand_states.alloc_info.size,
		},
	};

	const VkCopyBufferInfo2 copy_buff_info = {
		.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
		.srcBuffer = staging_buffer.buffer,
		.dstBuffer = rt_pipeline.rand_states.buffer,
		.regionCount = std::size(regions),
		.pRegions = regions,
	};

	vkCmdCopyBuffer2(xfer_cmd_buff, &copy_buff_info);

	VK_CHECK("end xfer cmd buff", vkEndCommandBuffer(xfer_cmd_buff));

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = xfer_cmd_buff,
		}
	};

	const VkSubmitInfo2 submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = std::size(cmd_buff_infos),
			.pCommandBufferInfos = cmd_buff_infos,
		},
	};

	VK_CHECK("submit xfer cmd buff", vkQueueSubmit2(xfer_q, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));
	VK_CHECK("wait for device", vkDeviceWaitIdle(device));
	vk_buffer::destroy(staging_buffer, allocator, device);
}

ray_tracer ray_tracer_create(const VkDevice device, const VkExtent3D& extent, const VmaAllocator& allocator, const std::string& current_path, const uint32_t q_fly_idx, const std::vector<uint32_t>& q_fly_idxs, const VkCommandBuffer xfer_cmd_buff, const VkQueue xfer_q, const std::string& name)
{
	ray_tracer d = {};

	d.cmd_buffs.resize(d.max_frames_in_flight);
	d.frame_sems.resize(d.max_frames_in_flight);
	d.frame_sem_vals.resize(d.max_frames_in_flight, 1);

	const VkCommandPoolCreateInfo cmd_pool_ci = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = q_fly_idx,
	};

	VK_CHECK("create command pool", vkCreateCommandPool(device, &cmd_pool_ci, nullptr, &d.cmd_pool));

	const VkCommandBufferAllocateInfo cmd_buff_ai = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = d.cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
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

	for (uint8_t i = 0; i < d.max_frames_in_flight; ++i)
	{
		VK_CHECK("allocate rt command buffer", vkAllocateCommandBuffers(device, &cmd_buff_ai, &d.cmd_buffs[i]));
		VK_CHECK("create rt frame semahpore", vkCreateSemaphore(device, &tl_sem_ci, nullptr, &d.frame_sems[i]));

		const VkSemaphoreSignalInfo signal_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
			.semaphore = d.frame_sems[i],
			.value = 1,
		};

		VK_CHECK("signal rt frame sem", vkSignalSemaphore(device, &signal_info));
	}

	d.accum_target = vk_image::create(device, extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, allocator, 0, "accum target", q_fly_idxs);
	d.final_render = vk_image::create(device, extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, allocator, 0, "final target", q_fly_idxs);
	d.rand_states = vk_buffer::create(device, allocator, sizeof(uint32_t) * 4 * extent.width * extent.height, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, "rand states");

	ray_tracer_initialize_resources(d, device, allocator, extent, xfer_cmd_buff, xfer_q);

#ifdef _DEBUG
	std::string n(name);
	VkDebugUtilsObjectNameInfoEXT name_info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.objectType = VK_OBJECT_TYPE_COMMAND_POOL,
		.objectHandle = reinterpret_cast<uint64_t>(d.cmd_pool),
		.pObjectName = n.append(" cmd pool").c_str(),
	};

	VK_CHECK("set cmpt cmd pool name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

	for (uint8_t i = 0; i < d.max_frames_in_flight; ++i)
	{
		n = name;
		name_info.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.cmd_buffs[i]);
		name_info.pObjectName = n.append(" cmpt cmd buff ").append(std::to_string(i)).c_str();
		VK_CHECK("setting swapchain cmpt command buffer name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

		n = name;
		name_info.objectType = VK_OBJECT_TYPE_SEMAPHORE;
		name_info.objectHandle = reinterpret_cast<uint64_t>(d.frame_sems[i]);
		name_info.pObjectName = n.append(" cmpt frame sem ").c_str();
		VK_CHECK("setting swapchain cmpt sig sem name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
	}

#endif	// _DEBUG

	return d;
}

void ray_tracer_recreate_render_targets(ray_tracer& rt, const VkDevice device, const VkExtent3D& extent, const VmaAllocator& allocator, const std::vector<uint32_t>& q_fly_idxs, const VkCommandBuffer xfer_cmd_buff, const VkQueue xfer_q)
{
	vk_image::destroy(rt.accum_target, allocator, device);
	rt.accum_target = vk_image::create(device, extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, allocator, 0, "accum target", q_fly_idxs);

	vk_image::destroy(rt.final_render, allocator, device);
	rt.final_render = vk_image::create(device, extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, allocator, 0, "final target", q_fly_idxs);

	ray_tracer_initialize_resources(rt, device, allocator, extent, xfer_cmd_buff, xfer_q);

	vk_buffer::destroy(rt.rand_states, allocator, device);
	rt.rand_states = vk_buffer::create(device, allocator, sizeof(uint32_t) * 4 * extent.width * extent.height, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, "rand states");
}

void ray_tracer_destroy(ray_tracer rt, const VmaAllocator& allocator, const VkDevice device)
{
	if (device != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(device, rt.cmd_pool, nullptr);

		for (auto& sem : rt.frame_sems)
			vkDestroySemaphore(device, sem, nullptr);

		vk_image::destroy(rt.accum_target, allocator, device);
		vk_image::destroy(rt.final_render, allocator, device);
		vk_buffer::destroy(rt.rand_states, allocator, device);
	}
}

