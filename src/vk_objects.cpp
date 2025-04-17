#include "vk_objects.hpp"

#include <Windows.h>

#include <iostream>
#include <vector>
#include <sstream>
#include <filesystem>
#include <fstream>

vk_instance::vk_instance()
{
    std::vector<const char*> req_ext_names = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME,
        VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME
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

    this->present_mode = VK_PRESENT_MODE_FIFO_KHR;
    this->instance = instance;
    this->h_wnd = h_wnd;
    this->h_instance = h_instance;
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
        VkPhysicalDeviceProperties2 props = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &desc_buff_props,
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
}

vk_device::vk_device(const VkPhysicalDevice phy_dev, const uint32_t q_fly_idx, const uint32_t q_count)
{
    const char* req_ext_names[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
        VK_EXT_MESH_SHADER_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
        VK_KHR_MAINTENANCE_4_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
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

    VkPhysicalDeviceMaintenance4Features main_4_feats = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES,
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

    VkPhysicalDeviceFeatures2 feats2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &sync2_feats,
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

    VK_CHECK("create device", vkCreateDevice(phy_dev, &create_info, nullptr, &device));
}

vk_device::~vk_device()
{
    if (device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device, nullptr);
    }
}

VkResult vk_device::wait_semaphores(const std::vector<VkSemaphore>& semaphores, const std::vector<uint64_t>& values) const
{
    const VkSemaphoreWaitInfo wait_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = static_cast<uint32_t>(semaphores.size()),
        .pSemaphores = semaphores.data(),
        .pValues = values.data(),
    };

    return vkWaitSemaphores(device, &wait_info, UINT64_MAX);
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
    cmd_buffs.resize(images_count);
    rndr_semaphores.resize(images_count);
    present_fences.resize(images_count);

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
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = phy_dev->q_fly_idx,
    };
    VK_CHECK("create command pool", vkCreateCommandPool(device, &cmd_pool_ci, nullptr, &cmd_pool));

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

    for (uint32_t i = 0; i < images_count; ++i)
    {
        image_view_create_info.image = images[i];
        VK_CHECK("create swapchain image view", vkCreateImageView(device, &image_view_create_info, nullptr, &image_views[i]));

        cmd_buff_ai.commandPool = cmd_pool;
        VK_CHECK("allocate command buffer", vkAllocateCommandBuffers(device, &cmd_buff_ai, &cmd_buffs[i]));
        VK_CHECK("create semaphore", vkCreateSemaphore(device, &sem_ci, nullptr, &rndr_semaphores[i]));
        VK_CHECK("create fence", vkCreateFence(device, &fence_ci, nullptr, &present_fences[i]));
    }

    this->device = device;
}

vk_swapchain::~vk_swapchain()
{
    vkDestroyCommandPool(device, cmd_pool, nullptr);

    for (uint32_t i = 0; i < images_count; ++i)
    {
        vkDestroyFence(device, present_fences[i], nullptr);
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

VkResult vk_semaphore::signal(const uint64_t value) const
{
    const VkSemaphoreSignalInfo signal_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
        .semaphore = semaphore,
    };

    return vkSignalSemaphore(this->device, &signal_info);
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

vk_command_pool::vk_command_pool(const VkDevice device, const uint32_t q_fly_idx, const uint32_t cmd_buffs_count)
{
    const VkCommandPoolCreateInfo cmd_pool_ci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = q_fly_idx,
    };

    VK_CHECK("create command pool", vkCreateCommandPool(device, &cmd_pool_ci, nullptr, &cmd_pool));

    const VkCommandBufferAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = cmd_buffs_count,
    };

    cmd_buffs.resize(cmd_buffs_count);

    VK_CHECK("allocate command buffer", vkAllocateCommandBuffers(device, &allocate_info, cmd_buffs.data()));

    this->device = device;
}

vk_command_pool::~vk_command_pool()
{
    if (cmd_pool != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, cmd_pool, nullptr);
    }
}

vk_command_buffer::vk_command_buffer(const VkDevice device, const VkCommandPool cmd_pool)
{
    const VkCommandBufferAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VK_CHECK("allocate command buffer", vkAllocateCommandBuffers(device, &allocate_info, &cmd_buff));

    this->cmd_pool = cmd_pool;
    this->device = device;
}

vk_command_buffer::~vk_command_buffer()
{
}

VkResult vk_command_buffer::begin() const
{
    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    return vkBeginCommandBuffer(cmd_buff, &begin_info);
}

VkResult vk_command_buffer::end() const
{
    return vkEndCommandBuffer(cmd_buff);
}

vk_buffer::vk_buffer(const VkDevice device, const VkDeviceSize size, const VkBufferUsageFlags usage)
{
    const VkBufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
    };

    VK_CHECK("create buffer", vkCreateBuffer(device, &create_info, nullptr, &buffer));

    this->device = device;
}

vk_buffer::~vk_buffer()
{
    if (buffer != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, buffer, nullptr);
    }
}

vk_device_memory::vk_device_memory(const VkDevice device, const VkDeviceSize size, const uint32_t type_id)
{
    const VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = size,
        .memoryTypeIndex = type_id,
    };

    VK_CHECK("allocate memory", vkAllocateMemory(device, &alloc_info, nullptr, &memory));

    this->device = device;
}

vk_device_memory::~vk_device_memory()
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

host_buffer_memory::host_buffer_memory(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize size, const VkBufferUsageFlags usage)
{
    buffer = std::make_unique<vk_buffer>(device, size, usage);
    const VkBufferMemoryRequirementsInfo2 buff_mem_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
        .buffer = buffer->buffer,
    };

    VkMemoryRequirements2 mem_reqs = { 
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
    };

    vkGetBufferMemoryRequirements2(device, &buff_mem_info, &mem_reqs);
    uint32_t mem_types = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    memory = std::make_unique<vk_device_memory>(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types));
    VK_CHECK("bind device buffer to memory", vkBindBufferMemory(device, buffer->buffer, memory->memory, 0));
}

host_buffer_memory::host_buffer_memory(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize offset, const VkBufferUsageFlags usage, const std::vector<uint8_t>& data)
{
    host_buffer_memory host_buff_mem = host_buffer_memory(device, mem_props, data.size(), usage);

    buffer = std::move(host_buff_mem.buffer);
    memory = std::move(host_buff_mem.memory);

    void* map = NULL;
    VK_CHECK("map memory", vkMapMemory(device, memory->memory, offset, data.size(), 0, &map));

    memcpy(map, data.data(), data.size());
    vkUnmapMemory(device, memory->memory);
}

device_buffer_memory::device_buffer_memory(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize size, const VkBufferUsageFlags usage)
{
    buffer = std::make_unique<vk_buffer>(device, size, usage);
    const VkBufferMemoryRequirementsInfo2 buff_mem_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
        .buffer = buffer->buffer,
    };

    VkMemoryRequirements2 mem_reqs = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
    };
    vkGetBufferMemoryRequirements2(device, &buff_mem_info, &mem_reqs);
    uint32_t mem_types = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    memory = std::make_unique<vk_device_memory>(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types));

    VK_CHECK("bind device buffer to memory", vkBindBufferMemory(device, buffer->buffer, memory->memory, 0));
}

device_buffer_memory::device_buffer_memory(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize offset, const VkBufferUsageFlags usage, const std::vector<uint8_t>& data, const VkQueue xfer_q, const VkCommandBuffer cmd_buff)
{
    host_buffer_memory host_buff_mem = host_buffer_memory(device, mem_props, offset, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, data);
    device_buffer_memory device_buff_mem = device_buffer_memory(device, mem_props, data.size(), usage);

    buffer = std::move(device_buff_mem.buffer);
    memory = std::move(device_buff_mem.memory);

    std::vector<VkBufferCopy> regions = {
        {
            .size = data.size(),
        },
    };

    copy_buffer_to_buffer(host_buff_mem.buffer->buffer, buffer->buffer, regions, cmd_buff, xfer_q);
}

vk_descriptor_set_layout::vk_descriptor_set_layout(const VkDevice device, const SpvReflectDescriptorSet* spv_dsl, const VkShaderStageFlags stage)
{
    std::vector<VkDescriptorSetLayoutBinding> bindings(spv_dsl->binding_count);
    const VkDescriptorSetLayoutCreateInfo dsl_ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = spv_dsl->binding_count,
        .pBindings = bindings.data(),
    };

    for (uint32_t b = 0; b < spv_dsl->binding_count; ++b)
    {
        bindings[b] = {
            .binding = spv_dsl->bindings[b]->binding,
            .descriptorType = static_cast<VkDescriptorType>(spv_dsl->bindings[b]->descriptor_type),
            .descriptorCount = spv_dsl->bindings[b]->count,
            .stageFlags = stage,
        };
    }

    VK_CHECK("create descriptor set layout", vkCreateDescriptorSetLayout(device, &dsl_ci, nullptr, &dsl));

    this->device = device;
}

vk_descriptor_set_layout::vk_descriptor_set_layout(vk_descriptor_set_layout&& other)
{
    dsl = other.dsl;
    device = other.device;

    other.dsl = VK_NULL_HANDLE;
    other.device = VK_NULL_HANDLE;
}

vk_descriptor_set_layout& vk_descriptor_set_layout::operator=(vk_descriptor_set_layout&& other)
{
    if (dsl != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(device, dsl, nullptr);
    }

    dsl = other.dsl;
    device = other.device;

    other.dsl = VK_NULL_HANDLE;
    other.device = VK_NULL_HANDLE;

    return *this;
}

vk_descriptor_set_layout::~vk_descriptor_set_layout()
{
    if (dsl != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(device, dsl, nullptr);
    }
}

vk_graphics_pipeline::vk_graphics_pipeline(const VkDevice device, const std::string& path, const CHI_PIPELINE_TYPE p_type, const VkFormat format)
{
    std::vector<VkPipelineShaderStageCreateInfo> stages;

    for (auto const& file : std::filesystem::directory_iterator(std::filesystem::path(path)))
    {
        if (file.path().extension().string() != ".spv")
            continue;

        std::vector<char> file_data(std::filesystem::file_size(file));
        std::ifstream spv_file(file.path(), std::ifstream::binary);
        spv_file.read(file_data.data(), file_data.size());

        SpvReflectShaderModule spv_module;
        SPV_CHECK("create spv module", spvReflectCreateShaderModule(file_data.size(), file_data.data(), &spv_module));

        uint32_t dsl_count = 0;
        SPV_CHECK("enumerate dsl", spvReflectEnumerateDescriptorSets(&spv_module, &dsl_count, nullptr));

        std::vector<SpvReflectDescriptorSet*> spv_dsls(dsl_count);
        SPV_CHECK("enumerate dsl", spvReflectEnumerateDescriptorSets(&spv_module, &dsl_count, spv_dsls.data()));

        for (const auto& spv_dsl : spv_dsls)
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings(spv_dsl->binding_count);
            const VkDescriptorSetLayoutCreateInfo dsl_ci = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = spv_dsl->binding_count,
                .pBindings = bindings.data(),
            };

            for (uint32_t b = 0; b < spv_dsl->binding_count; ++b)
            {
                bindings[b] = {
                    .binding = spv_dsl->bindings[b]->binding,
                    .descriptorType = static_cast<VkDescriptorType>(spv_dsl->bindings[b]->descriptor_type),
                    .descriptorCount = spv_dsl->bindings[b]->count,
                    .stageFlags = static_cast<VkShaderStageFlags>(spv_module.shader_stage),
                };
            }

            if (dsls.size() < spv_dsl->set + 1)
                dsls.resize(spv_dsl->set + 1);

            VK_CHECK("create descriptor set layout", vkCreateDescriptorSetLayout(device, &dsl_ci, nullptr, &dsls[spv_dsl->set]));
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

    const VkPipelineLayoutCreateInfo pl_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(dsls.size()),
        .pSetLayouts = dsls.data(),
    };

    VK_CHECK("create pipeline layout", vkCreatePipelineLayout(device, &pl_ci, nullptr, &pipeline_layout));

    const VkViewport viewports[] = {
        {}
    };

    const VkRect2D scissors[] = {
        {},
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

    const VkDynamicState ds[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH,
        VK_DYNAMIC_STATE_POLYGON_MODE_EXT,
    };

    const VkPipelineDynamicStateCreateInfo ds_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = _countof(ds),
        .pDynamicStates = ds,
    };

    const VkFormat col_attch_forms[] = {
        format,
    };

    const VkPipelineRenderingCreateInfo rend_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = _countof(col_attch_forms),
        .pColorAttachmentFormats = col_attch_forms,
    };

    if (p_type == CHI_PIPELINE_TYPE::PBR) {
        const VkGraphicsPipelineCreateInfo p_ci = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &rend_info,
            .stageCount = static_cast<uint32_t>(stages.size()),
            .pStages = stages.data(),
            .pViewportState = &vs_ci,
            .pRasterizationState = &rs_ci,
            .pMultisampleState = &ms_ci,
            .pColorBlendState = &cbs_ci,
            .pDynamicState = &ds_ci,
            .layout = pipeline_layout,
        };

        VK_CHECK("create pbr pipeline", vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &p_ci, nullptr, &pipeline));
    }

    this->device = device;

    for (auto& stage : stages)
    {
        vkDestroyShaderModule(device, stage.module, nullptr);
    }
}

vk_graphics_pipeline::~vk_graphics_pipeline()
{
    if (pipeline != VK_NULL_HANDLE && device != VK_NULL_HANDLE && pipeline_layout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);

        for (auto dsl : dsls)
        {
            vkDestroyDescriptorSetLayout(device, dsl, nullptr);
        }
    }
}

vk_pipeline_layout::~vk_pipeline_layout()
{
    if (pipeline_layout != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    }
}
