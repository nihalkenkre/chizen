#include "vk_objects.hpp"

#include <Windows.h>

#include <iostream>
#include <vector>
#include <sstream>
#include <filesystem>
#include <fstream>

inline static VkDeviceSize ALIGNED_SIZE(VkDeviceSize value, VkDeviceSize alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

VkInstance vk_instance::create()
{
    std::vector<const char*> req_ext_names = {
           VK_KHR_SURFACE_EXTENSION_NAME,
           VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
           VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME,
           VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
#ifdef DEBUG
           VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

    uint32_t property_count = 0;
    VK_CHECK("enumerate instance extensions", vkEnumerateInstanceExtensionProperties(nullptr, &property_count, nullptr));

    std::vector<VkExtensionProperties> props(property_count);
    VK_CHECK("enumerate instance extensions", vkEnumerateInstanceExtensionProperties(nullptr, &property_count, props.data()));

    for (auto const& req_ext_name : req_ext_names)
    {
        auto it = std::find_if(props.begin(), props.end(), [&](const VkExtensionProperties prop) { return (std::strcmp(prop.extensionName, req_ext_name) == 0); });

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
        .apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 296),
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

void vk_instance::destroy(const VkInstance instance)
{
    if (instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, nullptr);
    }
}

vk_surface::data vk_surface::create(const VkInstance instance, const HINSTANCE h_instance, const HWND h_wnd)
{
    const VkWin32SurfaceCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = h_instance,
        .hwnd = h_wnd
    };

    data d;

    VK_CHECK("create surface", vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &d.surface));

    d.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    d.h_wnd = h_wnd;
    d.h_instance = h_instance;

    return d;
}

void vk_surface::destroy(const VkSurfaceKHR surface, VkInstance instance)
{
    if (surface != VK_NULL_HANDLE && instance != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }
}

vk_phy_dev::data vk_phy_dev::get_phy_dev(const VkInstance instance, vk_surface::data* surface)
{
    uint32_t phy_dev_count = 0;
    VK_CHECK("enumerate physical devices", vkEnumeratePhysicalDevices(instance, &phy_dev_count, nullptr));

    std::vector<VkPhysicalDevice> phy_devs(phy_dev_count);
    VK_CHECK("enumerate physical devices", vkEnumeratePhysicalDevices(instance, &phy_dev_count, phy_devs.data()));

    vk_phy_dev::data d;

    for (auto const& phy_dev : phy_devs)
    {
        VkPhysicalDeviceProperties2 props = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &d.desc_buff_props,
        };
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
                    vkGetPhysicalDeviceWin32PresentationSupportKHR(phy_dev, q) &&
                    q_fly_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    d.phy_dev = phy_dev;
                    d.q_count = q_fly_props[q].queueCount;
                    d.q_fly_idx = q;
                    d.props = props;

                    vkGetPhysicalDeviceMemoryProperties(phy_dev, &d.mem_props);

                    VK_CHECK("get surface caps", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phy_dev, surface->surface, &surface->surf_caps));

                    {
                        uint32_t surf_forms_count = 0;
                        VK_CHECK("get surface formats", vkGetPhysicalDeviceSurfaceFormatsKHR(phy_dev, surface->surface, &surf_forms_count, nullptr));

                        std::vector<VkSurfaceFormatKHR> surf_forms(surf_forms_count);
                        VK_CHECK("get surface formats", vkGetPhysicalDeviceSurfaceFormatsKHR(phy_dev, surface->surface, &surf_forms_count, surf_forms.data()));

                        auto it = std::find_if(surf_forms.begin(), surf_forms.end(), [](const VkSurfaceFormatKHR frm) { return (frm.format == VK_FORMAT_R8G8B8A8_UNORM) && (frm.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR); });

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

                        auto it = std::find_if(present_modes.begin(), present_modes.end(), [](const VkPresentModeKHR mode) { return mode == VK_PRESENT_MODE_MAILBOX_KHR; });

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

VkDevice vk_device::create(const VkPhysicalDevice phy_dev, const uint32_t q_fly_idx, const uint32_t q_count)
{
    const char* req_ext_names[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
        VK_EXT_MESH_SHADER_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
        VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
#ifdef DESC_BUFFER
        VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
#endif // DESC_BUFFER
        VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
    };

    uint32_t properties_count = 0;
    VK_CHECK("enumerate dev ext props", vkEnumerateDeviceExtensionProperties(phy_dev, nullptr, &properties_count, nullptr));

    std::vector<VkExtensionProperties> props(properties_count);
    VK_CHECK("enumerate dev ext props", vkEnumerateDeviceExtensionProperties(phy_dev, nullptr, &properties_count, props.data()));

    for (auto const& req_ext_name : req_ext_names)
    {
        auto it = std::find_if(props.begin(), props.end(), [&](const VkExtensionProperties prop) { return (std::strcmp(prop.extensionName, req_ext_name) == 0); });

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

#ifdef DESC_BUFFER
    VkPhysicalDeviceDescriptorBufferFeaturesEXT desc_buff_feats = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
    };
#endif // DESC_BUFFER

    VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchain_main_1_feats = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT,
#ifdef DESC_BUFFER
        .pNext = &desc_buff_feats,
#endif // DESC_BUFFER
    };

    VkPhysicalDeviceRobustness2FeaturesEXT rob2_feats = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
        .pNext = &swapchain_main_1_feats,
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures bda_feats = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .pNext = &rob2_feats,
    };

    VkPhysicalDeviceMaintenance4Features main_4_feats = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES,
        .pNext = &bda_feats,
    };

    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT ext_dyn_3_feats = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT,
        .pNext = &main_4_feats,
    };

    VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shader_feats = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
        .pNext = &ext_dyn_3_feats,
    };

    VkPhysicalDeviceDynamicRenderingFeatures dyn_rend_feats = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .pNext = &mesh_shader_feats,
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

    vkGetPhysicalDeviceFeatures2(phy_dev, &feats2);

    mesh_shader_feats.multiviewMeshShader = VK_FALSE;
    mesh_shader_feats.primitiveFragmentShadingRateMeshShader = VK_FALSE;

    const VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &feats2,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &q_create_info,
        .enabledExtensionCount = _countof(req_ext_names),
        .ppEnabledExtensionNames = req_ext_names,
    };

    VkDevice device = VK_NULL_HANDLE;
    VK_CHECK("create device", vkCreateDevice(phy_dev, &create_info, nullptr, &device));

    return device;
}

void vk_device::destroy(const VkDevice device)
{
    if (device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device, nullptr);
    }
}

vk_swapchain::data vk_swapchain::create(const VkDevice device, const vk_surface::data& surface, const vk_phy_dev::data& phy_dev, const std::string& name)
{
    const VkSwapchainCreateInfoKHR create_info = {
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
        .components = VK_COMPONENT_SWIZZLE_IDENTITY,
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

#ifdef DEBUG
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
        name_info.objectType = VK_OBJECT_TYPE_IMAGE;
        name_info.objectHandle = reinterpret_cast<uint64_t>(d.images[i]);
        name_info.pObjectName = "swapchain image" + i;
        VK_CHECK("setting swapchain image name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

        name_info.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        name_info.objectHandle = reinterpret_cast<uint64_t>(d.image_views[i]);
        name_info.pObjectName = "swapchain image view" + i;
        VK_CHECK("setting swapcahin image view name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

        name_info.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
        name_info.objectHandle = reinterpret_cast<uint64_t>(d.cmd_buffs[i]);
        name_info.pObjectName = "swapchain command buffer " + i;
        VK_CHECK("setting swapchain command buffer name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

        name_info.objectType = VK_OBJECT_TYPE_SEMAPHORE;
        name_info.objectHandle = reinterpret_cast<uint64_t>(d.rndr_semaphores[i]);
        name_info.pObjectName = "swapchain render sem " + i;
        VK_CHECK("setting swapchain render sem name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

        name_info.objectType = VK_OBJECT_TYPE_FENCE;
        name_info.objectHandle = reinterpret_cast<uint64_t>(d.present_fences[i]);
        name_info.pObjectName = "swapchain present fence " + i;
        VK_CHECK("setting swapchain present fence name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
    }
#endif // DEBUG

    return d;
}

void vk_swapchain::destroy(vk_swapchain::data data, const VkDevice device)
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

vk_semaphore::data vk_semaphore::create(const VkDevice device, bool is_timeline, const std::string& name)
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

    data d = {
        .is_timeline = is_timeline,
    };

    VK_CHECK("create semaphore", vkCreateSemaphore(device, &create_info, nullptr, &d.semaphore));
    std::string s = name;

#ifdef DEBUG
    const VkDebugUtilsObjectNameInfoEXT name_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_SEMAPHORE,
        .objectHandle = reinterpret_cast<uint64_t>(d.semaphore),
        .pObjectName = name.c_str(),
    };
    VK_CHECK("set semaphore name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // DEBUG

    return d;
}

void vk_semaphore::destroy(const VkSemaphore semaphore, const VkDevice device)
{
    if (semaphore != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
}

//vk_fence::vk_fence(const VkDevice device, const VkBool32 signalled_state)
//{
//    const VkFenceCreateInfo create_info = {
//        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
//        .flags = signalled_state,
//    };
//
//    VK_CHECK("create fence", vkCreateFence(device, &create_info, nullptr, &fence));
//}
//
//vk_fence::~vk_fence()
//{
//    if (fence != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
//    {
//        vkDestroyFence(device, fence, nullptr);
//    }
//}

vk_command_pool::data vk_command_pool::create(const VkDevice device, const uint32_t q_fly_idx, const uint32_t cmd_buffs_count, const std::string& name)
{
    const VkCommandPoolCreateInfo cmd_pool_ci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
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

#ifdef DEBUG
    const VkDebugUtilsObjectNameInfoEXT name_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_COMMAND_POOL,
        .objectHandle = reinterpret_cast<uint64_t>(d.cmd_pool),
        .pObjectName = name.c_str(),
    };

    VK_CHECK("setting command pool name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // DEBUG

    return d;
}

void vk_command_pool::destroy(const VkCommandPool cmd_pool, const VkDevice device)
{
    if (cmd_pool != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, cmd_pool, nullptr);
    }
}

VkCommandBuffer vk_command_buffer::allocate(const VkDevice device, const VkCommandPool cmd_pool, const std::string& name)
{
    const VkCommandBufferAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer cmd_buff = VK_NULL_HANDLE;
    VK_CHECK("allocate command buffer", vkAllocateCommandBuffers(device, &allocate_info, &cmd_buff));

#ifdef DEBUG
    const VkDebugUtilsObjectNameInfoEXT name_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_COMMAND_BUFFER,
        .objectHandle = reinterpret_cast<uint64_t>(cmd_buff),
        .pObjectName = name.c_str(),
    };
    VK_CHECK("setting command buffer name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // DEBUG

    return cmd_buff;
}

VkBuffer vk_buffer::create(const VkDevice device, const VkDeviceSize size, const VkBufferUsageFlags usage, const std::string& name)
{
    const VkBufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
    };

    VkBuffer buffer = VK_NULL_HANDLE;
    VK_CHECK("create buffer", vkCreateBuffer(device, &create_info, nullptr, &buffer));

#ifdef DEBUG
    const VkDebugUtilsObjectNameInfoEXT name_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_BUFFER,
        .objectHandle = reinterpret_cast<uint64_t>(buffer),
        .pObjectName = name.c_str(),
    };
    VK_CHECK("setting buffer name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // DEBUG

    return buffer;
}

void vk_buffer::destroy(const VkBuffer buffer, const VkDevice device)
{
    if (buffer != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, buffer, nullptr);
    }
}

VkDeviceMemory vk_device_memory::allocate(const VkDevice device, const VkDeviceSize size, const uint32_t type_id, const std::string& name)
{
    const VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = size,
        .memoryTypeIndex = type_id,
    };

    VkDeviceMemory memory = VK_NULL_HANDLE;
    VK_CHECK("allocate memory", vkAllocateMemory(device, &alloc_info, nullptr, &memory));

#ifdef DEBUG
    const VkDebugUtilsObjectNameInfoEXT name_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_DEVICE_MEMORY,
        .objectHandle = reinterpret_cast<uint64_t>(memory),
        .pObjectName = name.c_str(),
    };
    VK_CHECK("setting device memory name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // DEBUG

    return memory;
}

VkDeviceMemory vk_device_memory::allocate(const VkDevice device, const VkDeviceSize size, const uint32_t type_id, const VkMemoryAllocateFlagsInfo& flags_info, const std::string& name)
{
    const VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &flags_info,
        .allocationSize = size,
        .memoryTypeIndex = type_id,
    };

    VkDeviceMemory memory = VK_NULL_HANDLE;
    VK_CHECK("allocate memory", vkAllocateMemory(device, &alloc_info, nullptr, &memory));

#ifdef DEBUG
    const VkDebugUtilsObjectNameInfoEXT name_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_DEVICE_MEMORY,
        .objectHandle = reinterpret_cast<uint64_t>(memory),
        .pObjectName = name.c_str(),
    };
    VK_CHECK("setting device memory name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // DEBUG

    return memory;
}

void vk_device_memory::free(const VkDeviceMemory memory, const VkDevice device)
{
    if (memory != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, memory, nullptr);
    }
}

static uint32_t get_memory_type_id(const VkPhysicalDeviceMemoryProperties mem_props, const VkMemoryRequirements mem_reqs, uint32_t mem_prop_types)
{
    uint32_t mem_id = UINT32_MAX;

    for (uint32_t mt = 0; mt < mem_props.memoryTypeCount; ++mt)
    {
        if (mem_reqs.memoryTypeBits & (1 << mt))
        {
            if (mem_props.memoryTypes[mt].propertyFlags & mem_prop_types)
            {
                if (mem_props.memoryHeaps[mem_props.memoryTypes[mt].heapIndex].size > mem_reqs.size)
                {
                    mem_id = mt;
                    break;
                }
            }
        }
    }

    return mem_id;
}

static void copy_buffer_to_buffer(VkBuffer src_buffer, const VkBuffer dst_buffer, const std::vector<VkBufferCopy> regions, const VkCommandBuffer cmd_buff, const VkQueue xfer_q)
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

    VK_CHECK("queue submit", vkQueueSubmit(xfer_q, _countof(submit_infos), submit_infos, VK_NULL_HANDLE));
    VK_CHECK("queue wait idle", vkQueueWaitIdle(xfer_q));
    VK_CHECK("reset buffer copy cmd buff", vkResetCommandBuffer(cmd_buff, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
}

host_buffer_memory::data host_buffer_memory::create(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize size, const VkBufferUsageFlags usage, const std::string& name)
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

        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), flags_info, name + " memory");
    }
    else {
        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), name + " memory");
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

host_buffer_memory::data host_buffer_memory::create(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize offset, const VkBufferUsageFlags usage, const std::vector<uint8_t>& data, const std::string& name)
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

        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), flags_info, name + " memory");
    }
    else {
        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), name + " memory");
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

void host_buffer_memory::destroy(const data bm, const VkDevice device)
{
    if (bm.buffer != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, bm.buffer, nullptr);
    }

    if (bm.memory != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, bm.memory, nullptr);
    }
}

device_buffer_memory::data device_buffer_memory::create(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize size, const VkBufferUsageFlags usage, const std::string& name)
{
    device_buffer_memory::data d;
    d.buffer = vk_buffer::create(device, size, usage, name + " buffer");
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

        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), flags_info, name + " memory");
    }
    else
    {
        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), name + " memory");
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

device_buffer_memory::data device_buffer_memory::create(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize offset, VkBufferUsageFlags usage, const std::vector<uint8_t>& data, const VkQueue xfer_q, const VkCommandBuffer cmd_buff, const std::string& name)
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

    //usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        const VkMemoryAllocateFlagsInfo flags_info = {
           .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
           .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        };

        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), flags_info, name + " memory");
    }
    else
    {
        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), name + " memory");
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

    std::vector<VkBufferCopy> regions = {
        {
            .size = data.size(),
        },
    };

    copy_buffer_to_buffer(host_buff_mem.buffer, d.buffer, regions, cmd_buff, xfer_q);

    host_buffer_memory::destroy(host_buff_mem, device);

    return d;

    //// device mem checker
    //host_buffer_memory checker = host_buffer_memory(device, mem_props, data.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    //copy_buffer_to_buffer(buffer->buffer, checker.buffer->buffer, regions, cmd_buff, xfer_q);

    //std::vector<uint8_t> checker_data(data.size());
    //void* map = nullptr;
    //VK_CHECK("map check memory", vkMapMemory(device, checker.memory->memory, offset, data.size(), 0, &map));
    //std::memcpy(checker_data.data(), map, data.size());

    //vkUnmapMemory(device, checker.memory->memory);
}

void device_buffer_memory::destroy(const data bm, const VkDevice device)
{
    if (bm.buffer != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, bm.buffer, nullptr);
    }

    if (bm.memory != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, bm.memory, nullptr);
    }
}

//vk_descriptor_set_layout::vk_descriptor_set_layout(const VkDevice device, const SpvReflectDescriptorSet* spv_dsl, const VkShaderStageFlags stage, const VkDeviceSize alignment)
//{
//    std::vector<VkDescriptorSetLayoutBinding> bindings(spv_dsl->binding_count);
//    const VkDescriptorSetLayoutCreateInfo dsl_ci = {
//        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
//        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
//        .bindingCount = spv_dsl->binding_count,
//        .pBindings = bindings.data(),
//    };
//
//    for (uint32_t b = 0; b < spv_dsl->binding_count; ++b)
//    {
//        bindings[b] = {
//            .binding = spv_dsl->bindings[b]->binding,
//            .descriptorType = static_cast<VkDescriptorType>(spv_dsl->bindings[b]->descriptor_type),
//            .descriptorCount = spv_dsl->bindings[b]->count,
//            .stageFlags = stage,
//        };
//    }
//
//    VK_CHECK("create descriptor set layout", vkCreateDescriptorSetLayout(device, &dsl_ci, nullptr, &dsl));
//
//    this->device = device;
//
//    vkGetDescriptorSetLayoutSizeEXT(device, dsl, &size);
//    size = ALIGNED_SIZE(size, alignment);
//
//    this->bindings.resize(spv_dsl->binding_count);
//
//    uint32_t b = 0;
//    for (auto& binding : this->bindings)
//    {
//        vkGetDescriptorSetLayoutBindingOffsetEXT(device, dsl, b, &binding.offset);
//        ++b;
//    }
//}

//vk_descriptor_set_layout::vk_descriptor_set_layout(vk_descriptor_set_layout&& other)
//{
//    dsl = other.dsl;
//    device = other.device;
//    bindings = std::move(other.bindings);
//    size = other.size;
//
//    other.dsl = VK_NULL_HANDLE;
//    other.device = VK_NULL_HANDLE;
//    other.bindings.clear();
//    other.size = 0;
//}
//
//vk_descriptor_set_layout& vk_descriptor_set_layout::operator=(vk_descriptor_set_layout&& other)
//{
//    if (dsl != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
//    {
//        vkDestroyDescriptorSetLayout(device, dsl, nullptr);
//    }
//
//    dsl = other.dsl;
//    device = other.device;
//    bindings = other.bindings;
//    size = other.size;
//
//    other.dsl = VK_NULL_HANDLE;
//    other.device = VK_NULL_HANDLE;
//    other.bindings.clear();
//    other.size = 0;
//
//    return *this;
//}
//
//vk_descriptor_set_layout::~vk_descriptor_set_layout()
//{
//    if (dsl != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
//    {
//        vkDestroyDescriptorSetLayout(device, dsl, nullptr);
//    }
//}

vk_graphics_pipeline::data vk_graphics_pipeline::create(const VkDevice device, const std::string& path, const CHI_PIPELINE_TYPE& p_type, VkFormat format, const VkPhysicalDeviceDescriptorBufferPropertiesEXT& desc_buff_props, const std::string& name)
{
    vk_graphics_pipeline::data d;

    std::vector<VkPipelineShaderStageCreateInfo> stages;
    std::vector<VkVertexInputBindingDescription> in_attr_bind_descs;
    std::vector<VkVertexInputAttributeDescription> in_attr_descs;
    std::vector<VkDescriptorSetLayoutCreateInfo> dsl_cis;
    std::vector<std::vector<VkDescriptorSetLayoutBinding>> dsl_cis_bindings;

    for (auto const& file : std::filesystem::directory_iterator(std::filesystem::path(path)))
    {
        if (file.path().extension().string() != ".spv")
            continue;

        std::vector<char> file_data(std::filesystem::file_size(file));
        std::ifstream spv_file(file.path(), std::ifstream::binary);
        spv_file.read(file_data.data(), file_data.size());

        SpvReflectShaderModule spv_module;
        SPV_CHECK("create spv module", spvReflectCreateShaderModule(file_data.size(), file_data.data(), &spv_module));

        if (spv_module.shader_stage & SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
        {
            uint32_t in_var_count = 0;
            SPV_CHECK("enumerate input vars", spvReflectEnumerateInputVariables(&spv_module, &in_var_count, nullptr));

            std::vector<SpvReflectInterfaceVariable*> in_vars(in_var_count);
            SPV_CHECK("enumerate input vars", spvReflectEnumerateInputVariables(&spv_module, &in_var_count, in_vars.data()));

            in_attr_bind_descs.resize(in_var_count);
            in_attr_descs.resize(in_var_count);

            for (uint32_t v = 0; v < in_var_count; ++v)
            {
                in_attr_bind_descs[v].binding = in_vars[v]->location;

                if (in_vars[v]->format == SPV_REFLECT_FORMAT_R32G32_SFLOAT)
                {
                    in_attr_bind_descs[v].stride = sizeof(float) * 2;
                }
                else if (in_vars[v]->format == SPV_REFLECT_FORMAT_R32G32B32_SFLOAT)
                {
                    in_attr_bind_descs[v].stride = sizeof(float) * 3;
                }
                else if (in_vars[v]->format == SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT)
                {
                    in_attr_bind_descs[v].stride = sizeof(float) * 4;
                }

                in_attr_descs[v].location = in_vars[v]->location;
                in_attr_descs[v].format = static_cast<VkFormat>(in_vars[v]->format);
                in_attr_descs[v].binding = in_vars[v]->location;
            }
        }

        uint32_t dsl_count = 0;
        SPV_CHECK("enumerate dsl", spvReflectEnumerateDescriptorSets(&spv_module, &dsl_count, nullptr));

        std::vector<SpvReflectDescriptorSet*> spv_dsls(dsl_count);
        SPV_CHECK("enumerate dsl", spvReflectEnumerateDescriptorSets(&spv_module, &dsl_count, spv_dsls.data()));

        for (const auto& spv_dsl : spv_dsls)
        {
            if (dsl_cis.size() < spv_dsl->set + 1)
            {
                dsl_cis.resize(spv_dsl->set + 1);
                dsl_cis_bindings.resize(spv_dsl->set + 1);
            }

            for (uint32_t b_idx = 0; b_idx < spv_dsl->binding_count; ++b_idx)
            {
                if (dsl_cis_bindings[spv_dsl->set].size() < spv_dsl->bindings[b_idx]->binding + 1)
                    dsl_cis_bindings[spv_dsl->set].resize(spv_dsl->bindings[b_idx]->binding + 1);

                dsl_cis_bindings[spv_dsl->set][spv_dsl->bindings[b_idx]->binding].binding = spv_dsl->bindings[b_idx]->binding;
                dsl_cis_bindings[spv_dsl->set][spv_dsl->bindings[b_idx]->binding].descriptorCount = spv_dsl->bindings[b_idx]->count;
                dsl_cis_bindings[spv_dsl->set][spv_dsl->bindings[b_idx]->binding].descriptorType = static_cast<VkDescriptorType>(spv_dsl->bindings[b_idx]->descriptor_type);
                dsl_cis_bindings[spv_dsl->set][spv_dsl->bindings[b_idx]->binding].stageFlags = spv_module.shader_stage;
            }
        }

        const VkShaderModuleCreateInfo sm_ci = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = file_data.size(),
            .pCode = reinterpret_cast<uint32_t*>(file_data.data())
        };

        VkShaderModule shader_module = VK_NULL_HANDLE;
        VK_CHECK("create shader module", vkCreateShaderModule(device, &sm_ci, nullptr, &shader_module));

        const VkPipelineShaderStageCreateInfo ss_ci = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = static_cast<VkShaderStageFlagBits> (spv_module.shader_stage),
            .module = shader_module,
            .pName = "main"
        };

        stages.push_back(ss_ci);

        spvReflectDestroyShaderModule(&spv_module);
    }

    d.dsls.resize(dsl_cis.size());
    d.dsl_infos.resize(dsl_cis.size());

    for (size_t dsl_ci_idx = 0; dsl_ci_idx < dsl_cis.size(); ++dsl_ci_idx)
    {
        dsl_cis[dsl_ci_idx].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
#ifdef  DESC_BUFFER
        dsl_cis[dsl_ci_idx].flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
#endif //  DESC_BUFFER
        dsl_cis[dsl_ci_idx].bindingCount = static_cast<uint32_t>(dsl_cis_bindings[dsl_ci_idx].size());
        dsl_cis[dsl_ci_idx].pBindings = dsl_cis_bindings[dsl_ci_idx].data();

        VK_CHECK("create desc set layout", vkCreateDescriptorSetLayout(device, &dsl_cis[dsl_ci_idx], nullptr, &d.dsls[dsl_ci_idx]));

        VkDeviceSize set_size = 0;
#ifdef DESC_BUFFER
        vkGetDescriptorSetLayoutSizeEXT(device, dsls[dsl_ci_idx], &set_size);
#endif // DESC_BUFFER

        d.dsl_infos[dsl_ci_idx].aligned_size = ALIGNED_SIZE(set_size, desc_buff_props.descriptorBufferOffsetAlignment);

        d.dsl_infos[dsl_ci_idx].binding_infos.resize(dsl_cis_bindings[dsl_ci_idx].size());

        for (uint32_t b_idx = 0; b_idx < dsl_cis_bindings[dsl_ci_idx].size(); ++b_idx)
        {
#ifdef DESC_BUFFER
            vkGetDescriptorSetLayoutBindingOffsetEXT(device, dsls[dsl_ci_idx], dsl_cis_bindings[dsl_ci_idx][b_idx].binding, &dsl_infos[dsl_ci_idx].binding_infos[dsl_cis_bindings[dsl_ci_idx][b_idx].binding].offset);
#endif // DESC_BUFFER
            d.dsl_infos[dsl_ci_idx].binding_infos[dsl_cis_bindings[dsl_ci_idx][b_idx].binding].type = dsl_cis_bindings[dsl_ci_idx][b_idx].descriptorType;
        }
    }

    const VkPipelineLayoutCreateInfo pl_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(d.dsls.size()),
        .pSetLayouts = d.dsls.data(),
    };

    VK_CHECK("create pipeline layout", vkCreatePipelineLayout(device, &pl_ci, nullptr, &d.pipeline_layout));

    const VkViewport viewports[] = {
        {}
    };

    const VkRect2D scissors[] = {
        {},
    };

    const VkPipelineVertexInputStateCreateInfo vis_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(in_attr_bind_descs.size()),
        .pVertexBindingDescriptions = in_attr_bind_descs.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(in_attr_descs.size()),
        .pVertexAttributeDescriptions = in_attr_descs.data(),
    };

    const VkPipelineInputAssemblyStateCreateInfo pias_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
    };

    const VkPipelineViewportStateCreateInfo vs_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = _countof(viewports),
        .pViewports = viewports,
        .scissorCount = _countof(scissors),
        .pScissors = scissors,
    };

    const VkPipelineRasterizationStateCreateInfo rs_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0,
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
        .attachmentCount = _countof(cbas),
        .pAttachments = cbas,
    };

    std::vector<VkDynamicState> ds = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH,
        VK_DYNAMIC_STATE_POLYGON_MODE_EXT,
    };

    if (p_type == CHI_PIPELINE_TYPE::VERTEX)
    {
        ds.push_back(VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY);
    }

    const VkPipelineDynamicStateCreateInfo ds_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(ds.size()),
        .pDynamicStates = ds.data(),
    };

    const VkFormat col_attch_forms[] = {
        format,
    };

    const VkPipelineRenderingCreateInfo rend_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = _countof(col_attch_forms),
        .pColorAttachmentFormats = col_attch_forms,
    };

    VkGraphicsPipelineCreateInfo p_ci = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rend_info,
        .stageCount = static_cast<uint32_t>(stages.size()),
        .pStages = stages.data(),
        .pVertexInputState = &vis_ci,
        .pInputAssemblyState = &pias_ci,
        .pViewportState = &vs_ci,
        .pRasterizationState = &rs_ci,
        .pMultisampleState = &ms_ci,
        .pColorBlendState = &cbs_ci,
        .pDynamicState = &ds_ci,
        .layout = d.pipeline_layout,
    };

#ifdef DESC_BUFFER
    p_ci.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
#endif // DESC_BUFFER

    VK_CHECK("create pbr pipeline", vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &p_ci, nullptr, &d.pipeline));

    d.p_type = p_type;

    for (auto& stage : stages)
    {
        vkDestroyShaderModule(device, stage.module, nullptr);
    }

#ifdef DEBUG
    VkDebugUtilsObjectNameInfoEXT name_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
    };
    for (const auto& dsl : d.dsls)
    {
        std::string tmp_name = name + " dsl";
        name_info.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
        name_info.objectHandle = reinterpret_cast<uint64_t>(dsl);
        name_info.pObjectName = tmp_name.c_str();
        VK_CHECK("setting descriptor set layout name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
    }

    std::string tmp_name = name + " pipeline layout";
    name_info.objectType = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
    name_info.objectHandle = reinterpret_cast<uint64_t>(d.pipeline_layout);
    name_info.pObjectName = tmp_name.c_str();
    VK_CHECK("setting pipeline layout name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

    name_info.objectType = VK_OBJECT_TYPE_PIPELINE;
    name_info.objectHandle = reinterpret_cast<uint64_t>(d.pipeline);
    name_info.pObjectName = name.c_str();
    VK_CHECK("setting pipeline name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

#endif // DEBUG


    return d;
}

void vk_graphics_pipeline::destroy(const data d, const VkDevice device)
{
    if (d.pipeline != VK_NULL_HANDLE && device != VK_NULL_HANDLE && d.pipeline_layout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, d.pipeline_layout, nullptr);
        vkDestroyPipeline(device, d.pipeline, nullptr);

        for (auto& dsl : d.dsls)
        {
            vkDestroyDescriptorSetLayout(device, dsl, nullptr);
        }
    }
}

VkDescriptorPool vk_descriptor_pool::create(const VkDevice& device, const uint32_t& max_sets, const std::vector<VkDescriptorPoolSize>& pool_sizes)
{
    const VkDescriptorPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = max_sets,
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };

    VkDescriptorPool desc_pool = VK_NULL_HANDLE;
    VK_CHECK("create descriptor pool", vkCreateDescriptorPool(device, &create_info, nullptr, &desc_pool));

    return desc_pool;
}

void vk_descriptor_pool::destroy(const VkDescriptorPool desc_pool, const VkDevice device)
{
    if (desc_pool != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, desc_pool, nullptr);
    }
}

std::vector<VkDescriptorSet> vk_descriptor_sets::allocate(const VkDevice& device, const VkDescriptorPool& desc_pool, const std::vector<VkDescriptorSetLayout>& desc_set_layouts)
{
    const VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = desc_pool,
        .descriptorSetCount = static_cast<uint32_t>(desc_set_layouts.size()),
        .pSetLayouts = desc_set_layouts.data(),
    };

    std::vector<VkDescriptorSet> desc_sets(desc_set_layouts.size());

    VK_CHECK("allocate descriptor sets", vkAllocateDescriptorSets(device, &alloc_info, desc_sets.data()));

    return desc_sets;
}

VkImage vk_image::create(const VkDevice device, const VkExtent3D& extent, const VkFormat format, const VkImageUsageFlags usage)
{
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

    VkImage image = VK_NULL_HANDLE;
    VK_CHECK("create image", vkCreateImage(device, &create_info, nullptr, &image));

    return image;
}

void vk_image::destroy(const VkImage image, const VkDevice device)
{
    if (image != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyImage(device, image, nullptr);
    }
}

VkImageView vk_image_view::create(const VkDevice device, const VkImage image, const VkImageViewType view_type, const VkFormat format)
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
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };

    VkImageView iv = VK_NULL_HANDLE;

    VK_CHECK("create image view", vkCreateImageView(device, &create_info, nullptr, &iv));

    return iv;
}

void vk_image_view::destroy(const VkImageView iv, const VkDevice device)
{
    if (iv != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, iv, nullptr);
    }
}
