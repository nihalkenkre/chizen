#include "vk_objects.hpp"

#include <Windows.h>

#include <iostream>
#include <vector>
#include <sstream>

vk_instance::vk_instance()
{
	std::vector<const char*> req_ext_names = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	};

	uint32_t property_count = 0;
	VK_CHECK("enumerate instance extensions", vkEnumerateInstanceExtensionProperties(nullptr, &property_count, nullptr));

	std::vector<VkExtensionProperties> props(property_count);
	VK_CHECK("enumerate instance extensions", vkEnumerateInstanceExtensionProperties(nullptr, &property_count, props.data()));

	for (auto const& req_ext_name : req_ext_names)
	{
		auto it = std::find_if(props.begin(), props.end(), [&](const VkExtensionProperties prop) { return (std::strcmp(prop.extensionName, req_ext_name) == 0);});

		if (it == props.end())
		{
			std::stringstream msg;
			msg << "Extension " << req_ext_name << " not supported by instance.\nExiting...\n";
#ifdef DEBUG 
			OutputDebugStringA(msg.str().c_str());
#else
			std::cout << msg.str();
#endif
			std::exit(-1);
		}
	}

	const VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Chizen",
		.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
		.pEngineName = "Chizen",
		.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
		.apiVersion = VK_MAKE_API_VERSION(0, 1, 4, 303),
	};

	const VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = static_cast<uint32_t>(req_ext_names.size()),
		.ppEnabledExtensionNames = req_ext_names.data(),
	};

	VK_CHECK("create instance", vkCreateInstance(&create_info, nullptr, &instance));
}

vk_instance::~vk_instance()
{
	if (instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(instance, nullptr);
	}
}

vk_surface::vk_surface(const VkInstance instance, const HINSTANCE h_instance, const HWND h_wnd) : format({}), surf_caps({})
{
	const VkWin32SurfaceCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = h_instance,
		.hwnd = h_wnd
	};

	VK_CHECK("create surface", vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &surface));

	this->present_mode = VK_PRESENT_MODE_FIFO_KHR,
		this->instance = instance;
}

vk_surface::~vk_surface()
{
	if (surface != VK_NULL_HANDLE && instance != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(instance, surface, nullptr);
	}
}

vk_phydev::vk_phydev(const VkInstance instance, vk_surface* surface)
{
	uint32_t phy_dev_count = 0;
	VK_CHECK("enumerate physical devices", vkEnumeratePhysicalDevices(instance, &phy_dev_count, nullptr));

	std::vector<VkPhysicalDevice> phy_devs(phy_dev_count);
	VK_CHECK("enumerate physical devices", vkEnumeratePhysicalDevices(instance, &phy_dev_count, phy_devs.data()));

	for (auto const& phy_dev : phy_devs)
	{
		VkPhysicalDeviceProperties props = {};
		vkGetPhysicalDeviceProperties(phy_dev, &props);

		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
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
					vkGetPhysicalDeviceWin32PresentationSupportKHR(phy_dev, q) &&
					q_fly_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					this->phy_dev = phy_dev;
					this->q_count = q_fly_props[q].queueCount;
					this->q_fly_idx = q;
					this->props = props;

					vkGetPhysicalDeviceMemoryProperties(phy_dev, &this->mem_props);

					VK_CHECK("get surface caps", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phy_dev, surface->surface, &surface->surf_caps));

					{
						uint32_t surf_forms_count = 0;
						VK_CHECK("get surface formats", vkGetPhysicalDeviceSurfaceFormatsKHR(phy_dev, surface->surface, &surf_forms_count, nullptr));

						std::vector<VkSurfaceFormatKHR> surf_forms(surf_forms_count);
						VK_CHECK("get surface formats", vkGetPhysicalDeviceSurfaceFormatsKHR(phy_dev, surface->surface, &surf_forms_count, surf_forms.data()));

						auto it = std::find_if(surf_forms.begin(), surf_forms.end(), [](const VkSurfaceFormatKHR frm) { return (frm.format == VK_FORMAT_R8G8B8A8_UNORM) && (frm.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR);});

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

						auto it = std::find_if(present_modes.begin(), present_modes.end(), [](const VkPresentModeKHR mode) { return mode == VK_PRESENT_MODE_MAILBOX_KHR;});

						if (it != present_modes.end())
						{
							surface->present_mode = *it;
						}
					}
				}
			}
		}
	}
}

vk_device::vk_device(const VkPhysicalDevice phy_dev, const uint32_t q_fly_idx, const uint32_t q_count)
{
	const char* req_ext_names[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
		VK_EXT_MESH_SHADER_EXTENSION_NAME,
	};

	uint32_t properties_count = 0;
	VK_CHECK("enumerate dev ext props", vkEnumerateDeviceExtensionProperties(phy_dev, nullptr, &properties_count, nullptr));

	std::vector<VkExtensionProperties> props(properties_count);
	VK_CHECK("enumerate dev ext props", vkEnumerateDeviceExtensionProperties(phy_dev, nullptr, &properties_count, props.data()));

	for (auto const& req_ext_name : req_ext_names)
	{
		auto it = std::find_if(props.begin(), props.end(), [&](const VkExtensionProperties prop) { return (std::strcmp(prop.extensionName, req_ext_name) == 0);});

		if (it == props.end())
		{
			std::stringstream msg;
			msg << "Extension " << req_ext_name << " not supported by physical device.\nExiting...\n";
#ifdef DEBUG 
			OutputDebugStringA(msg.str().c_str());
#else
			std::cout << msg.str();
#endif
			std::exit(-1);
		}
	}

	std::vector<float> priorities(q_count, 1);

	const VkDeviceQueueCreateInfo q_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = q_fly_idx,
		.queueCount = q_count,
		.pQueuePriorities = priorities.data(),
	};

	VkPhysicalDeviceDynamicRenderingFeatures dyn_rend_feats = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
	};

	VkPhysicalDeviceSynchronization2Features sync2_feats = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
		.pNext = &dyn_rend_feats,
	};

	VkPhysicalDeviceFeatures2 feats2 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &sync2_feats,
	};
	vkGetPhysicalDeviceFeatures2(phy_dev, &feats2);

	const VkDeviceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &feats2,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &q_create_info,
		.enabledExtensionCount = _countof(req_ext_names),
		.ppEnabledExtensionNames = req_ext_names,
	};

	VK_CHECK("create device", vkCreateDevice(phy_dev, &create_info, nullptr, &device));
}

vk_device::~vk_device()
{
	if (device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(device, nullptr);
	}
}

vk_swapchain::vk_swapchain(const VkDevice device, const vk_surface* surface, const vk_phydev* phy_dev)
{
	const VkSwapchainCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface->surface,
		.minImageCount = surface->surf_caps.minImageCount,
		.imageFormat = surface->format.format,
		.imageColorSpace = surface->format.colorSpace,
		.imageExtent = surface->surf_caps.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = surface->surf_caps.supportedUsageFlags,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &phy_dev->q_fly_idx,
		.preTransform = surface->surf_caps.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = surface->present_mode,
	};

	VK_CHECK("create swapchain", vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain));
	VK_CHECK("get swapchain images", vkGetSwapchainImagesKHR(device, swapchain, &images_count, nullptr));

	images.resize(images_count);
	VK_CHECK("get swapchain images", vkGetSwapchainImagesKHR(device, swapchain, &images_count, images.data()));

	image_views.resize(images_count);
	cmd_pools.resize(images_count);
	cmd_buffs.resize(images_count);
	rndr_semaphores.resize(images_count);

	VkImageViewCreateInfo image_view_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = surface->format.format,
		.components = VK_COMPONENT_SWIZZLE_IDENTITY,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};

	const VkCommandPoolCreateInfo cmd_pool_ci = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = phy_dev->q_fly_idx,
	};

	VkCommandBufferAllocateInfo cmd_buff_ai = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	const VkSemaphoreCreateInfo sem_ci = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	for (uint32_t i = 0; i < images_count; ++i)
	{
		image_view_create_info.image = images[i];
		VK_CHECK("create swapchain image view", vkCreateImageView(device, &image_view_create_info, nullptr, &image_views[i]));
		VK_CHECK("create command pool", vkCreateCommandPool(device, &cmd_pool_ci, nullptr, &cmd_pools[i]));

		cmd_buff_ai.commandPool = cmd_pools[i];
		VK_CHECK("allocate command buffer", vkAllocateCommandBuffers(device, &cmd_buff_ai, &cmd_buffs[i]));
		VK_CHECK("create semaphore", vkCreateSemaphore(device, &sem_ci, nullptr, &rndr_semaphores[i]));
	}

	this->device = device;
}

vk_swapchain::~vk_swapchain()
{
	for (uint32_t i = 0; i < images_count; ++i)
	{
		vkDestroyCommandPool(device, cmd_pools[i], nullptr);
		vkDestroyImageView(device, image_views[i], nullptr);
		vkDestroySemaphore(device, rndr_semaphores[i], nullptr);
	}

	if (swapchain != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(device, swapchain, nullptr);
	}
}

vk_semaphore::vk_semaphore(const VkDevice device, bool is_timeline)
{
	VkSemaphoreTypeCreateInfo t_ci = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		.initialValue = 0,
	};

	if (is_timeline)
	{
		t_ci.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	}
	else 
	{
		t_ci.semaphoreType = VK_SEMAPHORE_TYPE_BINARY;
	}

	const VkSemaphoreCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = &t_ci,
	};

	VK_CHECK("create semaphore", vkCreateSemaphore(device, &create_info, nullptr, &semaphore));

	this->device = device;
}

vk_semaphore::~vk_semaphore()
{
	if (semaphore != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(device, semaphore, nullptr);
	}
}

vk_fence::vk_fence(const VkDevice device, const VkBool32 signalled_state)
{
	const VkFenceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = signalled_state,
	};

	VK_CHECK("create fence", vkCreateFence(device, &create_info, nullptr, &fence));
}

vk_fence::~vk_fence()
{
	if (fence != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
	{
		vkDestroyFence(device, fence, nullptr);
	}
}

vk_command_buffers::vk_command_buffers(const VkDevice device)
{

}

vk_command_buffers::~vk_command_buffers()
{
}

vk_command_pool::vk_command_pool(const VkDevice device, const uint32_t q_fly_idx)
{
	const VkCommandPoolCreateInfo cmd_pool_ci = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = q_fly_idx,
	};

	VK_CHECK("create command pool", vkCreateCommandPool(device, &cmd_pool_ci, nullptr, &cmd_pool));

	this->device = device;
}

vk_command_pool::~vk_command_pool()
{
	if (cmd_pool != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(device, cmd_pool, nullptr);
	}
}
