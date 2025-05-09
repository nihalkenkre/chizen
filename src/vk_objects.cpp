#include "vk_objects.hpp"

#include <Windows.h>

#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>

inline static VkDeviceSize ALIGNED_SIZE(VkDeviceSize value, VkDeviceSize alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

uint32_t get_memory_type_id(const VkPhysicalDeviceMemoryProperties mem_props, const VkMemoryRequirements mem_reqs, uint32_t mem_prop_types)
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

void copy_buffer_to_buffer(const VkBuffer src_buffer, const VkBuffer dst_buffer, const std::vector<VkBufferCopy> regions, const VkCommandBuffer cmd_buff, const VkQueue xfer_q)
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK("begin command buffer", vkBeginCommandBuffer(cmd_buff, &begin_info));

    vkCmdCopyBuffer(cmd_buff, src_buffer, dst_buffer, static_cast<uint32_t>(regions.size()), regions.data());

    VK_CHECK("end command buffer", vkEndCommandBuffer(cmd_buff));

    std::vector<VkSubmitInfo> submit_infos(1);
    submit_infos[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_infos[0].commandBufferCount = 1;
    submit_infos[0].pCommandBuffers = &cmd_buff;

    VK_CHECK("queue submit", vkQueueSubmit(xfer_q, static_cast<uint32_t>(submit_infos.size()), submit_infos.data(), VK_NULL_HANDLE));
    VK_CHECK("queue wait idle", vkQueueWaitIdle(xfer_q));
    VK_CHECK("reset buffer copy cmd buff", vkResetCommandBuffer(cmd_buff, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
}

void copy_buffer_to_image(const VkBuffer src_buffer, const VkImage dst_image, const VkImageLayout dst_image_layout, const std::vector<VkBufferImageCopy2>& regions, const VkCommandBuffer cmd_buff, const VkQueue xfer_q)
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK("begin command buffer", vkBeginCommandBuffer(cmd_buff, &begin_info));

    VkImageMemoryBarrier2 img_copy_bar = {};
    img_copy_bar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    img_copy_bar.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    img_copy_bar.srcAccessMask = 0;
    img_copy_bar.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    img_copy_bar.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    img_copy_bar.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img_copy_bar.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    img_copy_bar.image = dst_image;
    img_copy_bar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    img_copy_bar.subresourceRange.levelCount = 1;
    img_copy_bar.subresourceRange.layerCount = 1;

    VkDependencyInfo dependency_info = {};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &img_copy_bar;
    vkCmdPipelineBarrier2(cmd_buff, &dependency_info);

    VkCopyBufferToImageInfo2 copy_info = {};
    copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
    copy_info.srcBuffer = src_buffer;
    copy_info.dstImage = dst_image;
    copy_info.dstImageLayout = dst_image_layout;
    copy_info.regionCount = static_cast<uint32_t>(regions.size());
    copy_info.pRegions = regions.data();
    vkCmdCopyBufferToImage2(cmd_buff, &copy_info);

    VkImageMemoryBarrier2 img_lyt_chng_bar = {};
    img_lyt_chng_bar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    img_lyt_chng_bar.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    img_lyt_chng_bar.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    img_lyt_chng_bar.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    img_lyt_chng_bar.dstAccessMask = 0;
    img_lyt_chng_bar.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    img_lyt_chng_bar.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    img_lyt_chng_bar.image = dst_image;
    img_lyt_chng_bar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    img_lyt_chng_bar.subresourceRange.levelCount = 1;
    img_lyt_chng_bar.subresourceRange.layerCount = 1;

    dependency_info.pImageMemoryBarriers = &img_lyt_chng_bar;
    vkCmdPipelineBarrier2(cmd_buff, &dependency_info);

    VK_CHECK("end command buffer", vkEndCommandBuffer(cmd_buff));

    std::vector<VkSubmitInfo> submit_infos(1);
    submit_infos[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_infos[0].commandBufferCount = 1;
    submit_infos[0].pCommandBuffers = &cmd_buff;

    VK_CHECK("queue submit", vkQueueSubmit(xfer_q, static_cast<uint32_t>(submit_infos.size()), submit_infos.data(), VK_NULL_HANDLE));
    VK_CHECK("queue wait idle", vkQueueWaitIdle(xfer_q));
    VK_CHECK("reset buffer copy cmd buff", vkResetCommandBuffer(cmd_buff, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
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

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Chizen";
    app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.pEngineName = "Chizen";
    app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    app_info.apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 296);

    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = static_cast<uint32_t>(req_ext_names.size());
    create_info.ppEnabledExtensionNames = req_ext_names.data();

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
    VkWin32SurfaceCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.hinstance = h_instance;
    create_info.hwnd = h_wnd;

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
        d.desc_buff_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;

        VkPhysicalDeviceProperties2 props;
        props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        props.pNext = &d.desc_buff_props,
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

    VkDeviceQueueCreateInfo q_create_info = {};
    q_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_create_info.queueFamilyIndex = q_fly_idx;
    q_create_info.queueCount = q_count;
    q_create_info.pQueuePriorities = priorities.data();

#ifdef DESC_BUFFER
    VkPhysicalDeviceDescriptorBufferFeaturesEXT desc_buff_feats = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
    };
#endif // DESC_BUFFER

    VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchain_main_1_feats = {};
    swapchain_main_1_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
#ifdef DESC_BUFFER
    swapchain_main_1_feats.pNext = &desc_buff_feats;
#endif // DESC_BUFFER

    VkPhysicalDeviceRobustness2FeaturesEXT rob2_feats = {};
    rob2_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
    rob2_feats.pNext = &swapchain_main_1_feats;

    VkPhysicalDeviceBufferDeviceAddressFeatures bda_feats = {};
    bda_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bda_feats.pNext = &rob2_feats;

    VkPhysicalDeviceMaintenance4Features main_4_feats = {};
    main_4_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES;
    main_4_feats.pNext = &bda_feats;

    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT ext_dyn_3_feats = {};
    ext_dyn_3_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
    ext_dyn_3_feats.pNext = &main_4_feats;

    VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shader_feats = {};
    mesh_shader_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
    mesh_shader_feats.pNext = &ext_dyn_3_feats;

    VkPhysicalDeviceDynamicRenderingFeatures dyn_rend_feats = {};
    dyn_rend_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dyn_rend_feats.pNext = &mesh_shader_feats;

    VkPhysicalDeviceSynchronization2Features sync2_feats = {};
    sync2_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2_feats.pNext = &dyn_rend_feats;

    VkPhysicalDeviceTimelineSemaphoreFeatures time_sem_feats = {};
    time_sem_feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
    time_sem_feats.pNext = &sync2_feats;

    VkPhysicalDeviceFeatures2 feats2 = {};
    feats2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    feats2.pNext = &time_sem_feats;

    vkGetPhysicalDeviceFeatures2(phy_dev, &feats2);

    mesh_shader_feats.multiviewMeshShader = VK_FALSE;
    mesh_shader_feats.primitiveFragmentShadingRateMeshShader = VK_FALSE;

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pNext = &feats2;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &q_create_info;
    create_info.enabledExtensionCount = _countof(req_ext_names);
    create_info.ppEnabledExtensionNames = req_ext_names;

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
    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface.surface;
    create_info.minImageCount = surface.surf_caps.minImageCount;
    create_info.imageFormat = surface.format.format;
    create_info.imageColorSpace = surface.format.colorSpace;
    create_info.imageExtent = surface.surf_caps.currentExtent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = surface.surf_caps.supportedUsageFlags;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices = &phy_dev.q_fly_idx;
    create_info.preTransform = surface.surf_caps.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = surface.present_mode;

    vk_swapchain::data d;
    VK_CHECK("create swapchain", vkCreateSwapchainKHR(device, &create_info, nullptr, &d.swapchain));
    VK_CHECK("get swapchain images", vkGetSwapchainImagesKHR(device, d.swapchain, &d.images_count, nullptr));

    d.images.resize(d.images_count);
    VK_CHECK("get swapchain images", vkGetSwapchainImagesKHR(device, d.swapchain, &d.images_count, d.images.data()));

    d.image_views.resize(d.images_count);
    d.cmd_buffs.resize(d.images_count);
    d.rndr_semaphores.resize(d.images_count);
    d.present_fences.resize(d.images_count);

    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = surface.format.format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.layerCount = 1;

    VkCommandPoolCreateInfo cmd_pool_ci = {};
    cmd_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_ci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_ci.queueFamilyIndex = phy_dev.q_fly_idx;
    VK_CHECK("create command pool", vkCreateCommandPool(device, &cmd_pool_ci, nullptr, &d.cmd_pool));

    VkCommandBufferAllocateInfo cmd_buff_ai = {};
    cmd_buff_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buff_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buff_ai.commandBufferCount = 1;

    VkSemaphoreCreateInfo sem_ci = {};
    sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_ci = {};
    fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

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
    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR;
    name_info.objectHandle = reinterpret_cast<uint64_t>(d.swapchain);
    name_info.pObjectName = name.c_str();
    VK_CHECK("set swapchain name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

    name_info.objectType = VK_OBJECT_TYPE_COMMAND_POOL;
    name_info.objectHandle = reinterpret_cast<uint64_t>(d.cmd_pool);
    name_info.pObjectName = "swapchain command pool";

    for (uint32_t i = 0; i < d.images_count; ++i)
    {
        char i_str[16];
        char object_name[128] = "swapchain image ";

        name_info.objectType = VK_OBJECT_TYPE_IMAGE;
        name_info.objectHandle = reinterpret_cast<uint64_t>(d.images[i]);
        name_info.pObjectName = std::strcat(object_name, _itoa(i, i_str, 10));
        VK_CHECK("setting swapchain image name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

        std::strcpy(object_name, "swapchain image view ");
        name_info.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        name_info.objectHandle = reinterpret_cast<uint64_t>(d.image_views[i]);
        name_info.pObjectName = std::strcat(object_name, _itoa(i, i_str, 10));
        VK_CHECK("setting swapcahin image view name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

        std::strcpy(object_name, "swapchain command buffer ");
        name_info.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
        name_info.objectHandle = reinterpret_cast<uint64_t>(d.cmd_buffs[i]);
        name_info.pObjectName = std::strcat(object_name, _itoa(i, i_str, 10));
        VK_CHECK("setting swapchain command buffer name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

        std::strcpy(object_name, "swapchain render semaphore ");
        name_info.objectType = VK_OBJECT_TYPE_SEMAPHORE;
        name_info.objectHandle = reinterpret_cast<uint64_t>(d.rndr_semaphores[i]);
        name_info.pObjectName = std::strcat(object_name, _itoa(i, i_str, 10));
        VK_CHECK("setting swapchain render sem name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

        std::strcpy(object_name, "swapchain present fence ");
        name_info.objectType = VK_OBJECT_TYPE_FENCE;
        name_info.objectHandle = reinterpret_cast<uint64_t>(d.present_fences[i]);
        name_info.pObjectName = std::strcat(object_name, _itoa(i, i_str, 10));
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

vk_semaphore::data vk_semaphore::create(const VkDevice device, const VkSemaphoreType semaphore_type, const std::string& name)
{
    VkSemaphoreTypeCreateInfo t_ci = {};
    t_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    t_ci.initialValue = 0;
    t_ci.semaphoreType = semaphore_type;

    VkSemaphoreCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    create_info.pNext = &t_ci;

    data d = {};
    d.type = semaphore_type;

    VK_CHECK("create semaphore", vkCreateSemaphore(device, &create_info, nullptr, &d.semaphore));
    std::string s = name;

#ifdef DEBUG
    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.objectType = VK_OBJECT_TYPE_SEMAPHORE;
    name_info.objectHandle = reinterpret_cast<uint64_t>(d.semaphore);
    name_info.pObjectName = name.c_str();
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
    VkCommandPoolCreateInfo cmd_pool_ci = {};
    cmd_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_ci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_ci.queueFamilyIndex = q_fly_idx;

    vk_command_pool::data d;

    VK_CHECK("create command pool", vkCreateCommandPool(device, &cmd_pool_ci, nullptr, &d.cmd_pool));

    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = d.cmd_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = cmd_buffs_count;

    d.cmd_buffs.resize(cmd_buffs_count);

    VK_CHECK("allocate command buffer", vkAllocateCommandBuffers(device, &allocate_info, d.cmd_buffs.data()));

#ifdef DEBUG
    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.objectType = VK_OBJECT_TYPE_COMMAND_POOL;
    name_info.objectHandle = reinterpret_cast<uint64_t>(d.cmd_pool);
    name_info.pObjectName = name.c_str();

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
    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = cmd_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer cmd_buff = VK_NULL_HANDLE;
    VK_CHECK("allocate command buffer", vkAllocateCommandBuffers(device, &allocate_info, &cmd_buff));

#ifdef DEBUG
    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
    name_info.objectHandle = reinterpret_cast<uint64_t>(cmd_buff);
    name_info.pObjectName = name.c_str();
    VK_CHECK("setting command buffer name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // DEBUG

    return cmd_buff;
}

VkBuffer vk_buffer::create(const VkDevice device, const VkDeviceSize size, const VkBufferUsageFlags usage, const std::string& name)
{
    VkBufferCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;

    VkBuffer buffer = VK_NULL_HANDLE;
    VK_CHECK("create buffer", vkCreateBuffer(device, &create_info, nullptr, &buffer));

#ifdef DEBUG
    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.objectType = VK_OBJECT_TYPE_BUFFER;
    name_info.objectHandle = reinterpret_cast<uint64_t>(buffer);
    name_info.pObjectName = name.c_str();

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
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = size;
    alloc_info.memoryTypeIndex = type_id;

    VkDeviceMemory memory = VK_NULL_HANDLE;
    VK_CHECK("allocate memory", vkAllocateMemory(device, &alloc_info, nullptr, &memory));

#ifdef DEBUG
    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY;
    name_info.objectHandle = reinterpret_cast<uint64_t>(memory);
    name_info.pObjectName = name.c_str();

    VK_CHECK("setting device memory name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // DEBUG

    return memory;
}

VkDeviceMemory vk_device_memory::allocate(const VkDevice device, const VkDeviceSize size, const uint32_t type_id, const VkMemoryAllocateFlagsInfo& flags_info, const std::string& name)
{
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = &flags_info;
    alloc_info.allocationSize = size;
    alloc_info.memoryTypeIndex = type_id;

    VkDeviceMemory memory = VK_NULL_HANDLE;
    VK_CHECK("allocate memory", vkAllocateMemory(device, &alloc_info, nullptr, &memory));

#ifdef DEBUG
    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY;
    name_info.objectHandle = reinterpret_cast<uint64_t>(memory);
    name_info.pObjectName = name.c_str();

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

host_buffer_memory::data host_buffer_memory::create(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize size, const VkBufferUsageFlags usage, const std::string& name)
{
    host_buffer_memory::data d;

    d.buffer = vk_buffer::create(device, size, usage, name + " buffer");
    VkBufferMemoryRequirementsInfo2 buff_mem_info = {};
    buff_mem_info.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
    buff_mem_info.buffer = d.buffer;

    VkMemoryRequirements2 mem_reqs = {};
    mem_reqs.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    vkGetBufferMemoryRequirements2(device, &buff_mem_info, &mem_reqs);
    uint32_t mem_types = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        VkMemoryAllocateFlagsInfo flags_info = {};
        flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), flags_info, name + " memory");
    }
    else {
        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), name + " memory");
    }

    VK_CHECK("bind device buffer to memory", vkBindBufferMemory(device, d.buffer, d.memory, 0));

    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        VkBufferDeviceAddressInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = d.buffer;

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
    VkBufferMemoryRequirementsInfo2 buff_mem_info = {};
    buff_mem_info.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
    buff_mem_info.buffer = d.buffer;

    VkMemoryRequirements2 mem_reqs = {};
    mem_reqs.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    vkGetBufferMemoryRequirements2(device, &buff_mem_info, &mem_reqs);
    uint32_t mem_types = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        VkMemoryAllocateFlagsInfo flags_info = {};
        flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), flags_info, name + " memory");
    }
    else {
        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), name + " memory");
    }

    VK_CHECK("bind device buffer to memory", vkBindBufferMemory(device, d.buffer, d.memory, 0));

    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        VkBufferDeviceAddressInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = d.buffer;

        d.addr = vkGetBufferDeviceAddress(device, &info);
        d.usage = usage;
    }

    VK_CHECK("map memory", vkMapMemory(device, d.memory, offset, data.size(), 0, &d.map));

    memcpy(d.map, data.data(), data.size());
    VkMappedMemoryRange mem_range = {};
    mem_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mem_range.memory = d.memory;
    mem_range.offset = offset;
    mem_range.size = data.size();
    VK_CHECK("flush memory", vkFlushMappedMemoryRanges(device, 1, &mem_range));

    return d;
}

void host_buffer_memory::destroy(const data bm, const VkDevice device)
{
    vk_buffer::destroy(bm.buffer, device);
    vk_device_memory::free(bm.memory, device);
}

device_buffer_memory::data device_buffer_memory::create(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize size, const VkBufferUsageFlags usage, const std::string& name)
{
    device_buffer_memory::data d;
    d.buffer = vk_buffer::create(device, size, usage, name + " buffer");
    VkBufferMemoryRequirementsInfo2 buff_mem_info = {};
    buff_mem_info.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
    buff_mem_info.buffer = d.buffer;

    VkMemoryRequirements2 mem_reqs = {};
    mem_reqs.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    vkGetBufferMemoryRequirements2(device, &buff_mem_info, &mem_reqs);
    uint32_t mem_types = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        VkMemoryAllocateFlagsInfo flags_info = {};
        flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), flags_info, name + " memory");
    }
    else
    {
        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), name + " memory");
    }

    VK_CHECK("bind device buffer to memory", vkBindBufferMemory(device, d.buffer, d.memory, 0));

    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        VkBufferDeviceAddressInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = d.buffer;

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
    VkBufferMemoryRequirementsInfo2 buff_mem_info = {};
    buff_mem_info.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
    buff_mem_info.buffer = d.buffer;

    VkMemoryRequirements2 mem_reqs = {};
    mem_reqs.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    vkGetBufferMemoryRequirements2(device, &buff_mem_info, &mem_reqs);
    uint32_t mem_types = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    //// device mem checker
    //usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        VkMemoryAllocateFlagsInfo flags_info = {};
        flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), flags_info, name + " memory");
    }
    else
    {
        d.memory = vk_device_memory::allocate(device, mem_reqs.memoryRequirements.size, get_memory_type_id(mem_props, mem_reqs.memoryRequirements, mem_types), name + " memory");
    }

    VK_CHECK("bind device buffer to memory", vkBindBufferMemory(device, d.buffer, d.memory, 0));

    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        VkBufferDeviceAddressInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = d.buffer;

        d.addr = vkGetBufferDeviceAddress(device, &info);
        d.usage = usage;
    }

    std::vector<VkBufferCopy> regions(1);
    regions[0].size = data.size();

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
    vk_buffer::destroy(bm.buffer, device);
    vk_device_memory::free(bm.memory, device);
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

    WIN32_FIND_DATAA find_data = {};
    std::string file_path = path + "*.spv";
    HANDLE h_find = FindFirstFileA(file_path.c_str(), &find_data);

    if (h_find == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Could not find shaders..." << GetLastError() << '\n';
        std::exit(GetLastError());
    }

    do {
        std::vector<char> file_data(find_data.nFileSizeLow);
        std::ifstream spv_file((path + (find_data.cFileName)).c_str(), std::ifstream::binary);
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
                if (dsl_cis_bindings[spv_dsl->set].size() < static_cast<size_t>(spv_dsl->bindings[b_idx]->binding) + 1)
                    dsl_cis_bindings[spv_dsl->set].resize(static_cast<size_t>(spv_dsl->bindings[b_idx]->binding) + 1);

                dsl_cis_bindings[spv_dsl->set][spv_dsl->bindings[b_idx]->binding].binding = spv_dsl->bindings[b_idx]->binding;
                dsl_cis_bindings[spv_dsl->set][spv_dsl->bindings[b_idx]->binding].descriptorCount = spv_dsl->bindings[b_idx]->count;
                dsl_cis_bindings[spv_dsl->set][spv_dsl->bindings[b_idx]->binding].descriptorType = static_cast<VkDescriptorType>(spv_dsl->bindings[b_idx]->descriptor_type);
                dsl_cis_bindings[spv_dsl->set][spv_dsl->bindings[b_idx]->binding].stageFlags = spv_module.shader_stage;
            }
        }

        VkShaderModuleCreateInfo sm_ci = {};
        sm_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        sm_ci.codeSize = file_data.size();
        sm_ci.pCode = reinterpret_cast<uint32_t*>(file_data.data());

        VkShaderModule shader_module = VK_NULL_HANDLE;
        VK_CHECK("create shader module", vkCreateShaderModule(device, &sm_ci, nullptr, &shader_module));

        VkPipelineShaderStageCreateInfo ss_ci = {};
        ss_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ss_ci.stage = static_cast<VkShaderStageFlagBits> (spv_module.shader_stage);
        ss_ci.module = shader_module;
        ss_ci.pName = "main";

        stages.push_back(ss_ci);

        spvReflectDestroyShaderModule(&spv_module);
    } while (FindNextFileA(h_find, &find_data) != 0);

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

    VkPipelineLayoutCreateInfo pl_ci = {};
    pl_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl_ci.setLayoutCount = static_cast<uint32_t>(d.dsls.size());
    pl_ci.pSetLayouts = d.dsls.data();

    VK_CHECK("create pipeline layout", vkCreatePipelineLayout(device, &pl_ci, nullptr, &d.pipeline_layout));

    const VkViewport viewports[] = {
        {}
    };

    const VkRect2D scissors[] = {
        {},
    };

    VkPipelineVertexInputStateCreateInfo vis_ci = {};
    vis_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vis_ci.vertexBindingDescriptionCount = static_cast<uint32_t>(in_attr_bind_descs.size());
    vis_ci.pVertexBindingDescriptions = in_attr_bind_descs.data();
    vis_ci.vertexAttributeDescriptionCount = static_cast<uint32_t>(in_attr_descs.size());
    vis_ci.pVertexAttributeDescriptions = in_attr_descs.data();

    VkPipelineInputAssemblyStateCreateInfo pias_ci = {};
    pias_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

    VkPipelineViewportStateCreateInfo vs_ci = {};
    vs_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vs_ci.viewportCount = _countof(viewports);
    vs_ci.pViewports = viewports;
    vs_ci.scissorCount = _countof(scissors);
    vs_ci.pScissors = scissors;

    VkPipelineRasterizationStateCreateInfo rs_ci = {};
    rs_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs_ci.polygonMode = VK_POLYGON_MODE_FILL;
    rs_ci.cullMode = VK_CULL_MODE_BACK_BIT;
    rs_ci.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs_ci.lineWidth = 1.0;

    VkPipelineMultisampleStateCreateInfo ms_ci = {};
    ms_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    std::vector<VkPipelineColorBlendAttachmentState> cbas(1);
    cbas[0].blendEnable = VK_TRUE;
    cbas[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cbas[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cbas[0].colorBlendOp = VK_BLEND_OP_ADD;
    cbas[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cbas[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cbas[0].alphaBlendOp = VK_BLEND_OP_ADD;
    cbas[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cbs_ci = {};
    cbs_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbs_ci.attachmentCount = static_cast<uint32_t>(cbas.size());
    cbs_ci.pAttachments = cbas.data();

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

    VkPipelineDynamicStateCreateInfo ds_ci = {};
    ds_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    ds_ci.dynamicStateCount = static_cast<uint32_t>(ds.size());
    ds_ci.pDynamicStates = ds.data();

    std::vector<VkFormat>col_attch_forms(1);
    col_attch_forms[0] = format;

    VkPipelineRenderingCreateInfo rend_info = {};
    rend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rend_info.colorAttachmentCount = static_cast<uint32_t>(col_attch_forms.size());
    rend_info.pColorAttachmentFormats = col_attch_forms.data();

    VkGraphicsPipelineCreateInfo p_ci = {};
    p_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    p_ci.pNext = &rend_info;
    p_ci.stageCount = static_cast<uint32_t>(stages.size());
    p_ci.pStages = stages.data();
    p_ci.pVertexInputState = &vis_ci;
    p_ci.pInputAssemblyState = &pias_ci;
    p_ci.pViewportState = &vs_ci;
    p_ci.pRasterizationState = &rs_ci;
    p_ci.pMultisampleState = &ms_ci;
    p_ci.pColorBlendState = &cbs_ci;
    p_ci.pDynamicState = &ds_ci;
    p_ci.layout = d.pipeline_layout;

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
    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;

    for (size_t i = 0; i < dsl_cis.size(); ++i)
    {
        char i_str[8];
        _itoa(static_cast<int>(i), i_str, 10);
        std::string tmp_name = name + " dsl " + i_str;
        name_info.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
        name_info.objectHandle = reinterpret_cast<uint64_t>(d.dsls[i]);
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

VkDescriptorPool vk_descriptor_pool::create(const VkDevice& device, const uint32_t& max_sets, const std::vector<VkDescriptorPoolSize>& pool_sizes, const std::string& name)
{
    VkDescriptorPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.maxSets = max_sets;
    create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    create_info.pPoolSizes = pool_sizes.data();

    VkDescriptorPool desc_pool = VK_NULL_HANDLE;
    VK_CHECK("create descriptor pool", vkCreateDescriptorPool(device, &create_info, nullptr, &desc_pool));

#ifdef DEBUG
    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.objectType = VK_OBJECT_TYPE_DESCRIPTOR_POOL;
    name_info.objectHandle = reinterpret_cast<uint64_t>(desc_pool);
    name_info.pObjectName = name.c_str();

    VK_CHECK("setting descriptor pool name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

#endif // DEBUG

    return desc_pool;
}

void vk_descriptor_pool::destroy(const VkDescriptorPool desc_pool, const VkDevice device)
{
    if (desc_pool != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, desc_pool, nullptr);
    }
}

std::vector<VkDescriptorSet> vk_descriptor_sets::allocate(const VkDevice& device, const VkDescriptorPool& desc_pool, const std::vector<VkDescriptorSetLayout>& desc_set_layouts, const std::string& name)
{
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = desc_pool;
    alloc_info.descriptorSetCount = static_cast<uint32_t>(desc_set_layouts.size());
    alloc_info.pSetLayouts = desc_set_layouts.data();

    std::vector<VkDescriptorSet> desc_sets(desc_set_layouts.size());

    VK_CHECK("allocate descriptor sets", vkAllocateDescriptorSets(device, &alloc_info, desc_sets.data()));

#ifdef DEBUG

    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;

    for (size_t i = 0; i < desc_set_layouts.size(); ++i)
    {
        char i_str[8];
        _itoa(static_cast<int>(i), i_str, 10);

        char name_str[128];
        std::strcpy(name_str, name.c_str());

        name_info.objectHandle = reinterpret_cast<uint64_t>(desc_sets[i]);
        name_info.pObjectName = std::strcat(name_str, i_str);
        VK_CHECK("setting descriptor set name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
    }

#endif // DEBUG

    return desc_sets;
}

VkImage vk_image::create(const VkDevice device, const VkExtent3D& extent, const VkFormat format, const VkImageUsageFlags usage, const std::string& name)
{
    VkImageCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.format = format;
    create_info.extent = extent;
    create_info.mipLevels = 1;
    create_info.arrayLayers = 1;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = usage;

    VkImage image = VK_NULL_HANDLE;
    VK_CHECK("create image", vkCreateImage(device, &create_info, nullptr, &image));

#ifdef  DEBUG
    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.objectType = VK_OBJECT_TYPE_IMAGE;
    name_info.objectHandle = reinterpret_cast<uint64_t>(image);
    name_info.pObjectName = name.c_str();

    VK_CHECK("setting image name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif //  DEBUG

    return image;
}

void vk_image::destroy(const VkImage image, const VkDevice device)
{
    if (image != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyImage(device, image, nullptr);
    }
}

VkImageView vk_image_view::create(const VkDevice device, const VkImage image, const VkImageViewType view_type, const VkFormat format, const std::string& name)
{
    VkImageViewCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image;
    create_info.viewType = view_type;
    create_info.format = format;
    create_info.components.r = VK_COMPONENT_SWIZZLE_R;
    create_info.components.g = VK_COMPONENT_SWIZZLE_G;
    create_info.components.b = VK_COMPONENT_SWIZZLE_B;
    create_info.components.a = VK_COMPONENT_SWIZZLE_A;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.layerCount = 1;

    VkImageView iv = VK_NULL_HANDLE;

    VK_CHECK("create image view", vkCreateImageView(device, &create_info, nullptr, &iv));

#ifdef  DEBUG
    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
    name_info.objectHandle = reinterpret_cast<uint64_t>(iv);
    name_info.pObjectName = name.c_str();

    VK_CHECK("setting image view name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif //  DEBUG

    return iv;
}

void vk_image_view::destroy(const VkImageView iv, const VkDevice device)
{
    if (iv != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, iv, nullptr);
    }
}

VkSampler vk_sampler::create(const VkDevice device, const cgltf_filter_type min_filter, const cgltf_filter_type mag_filter, const cgltf_wrap_mode wrap_s, const cgltf_wrap_mode wrap_t, const float& max_anisotropy, const float min_lod, const float max_lod, const std::string& name)
{
    VkSamplerCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    create_info.anisotropyEnable = VK_TRUE;
    create_info.maxAnisotropy = max_anisotropy;
    create_info.compareEnable = VK_FALSE;

    switch (min_filter)
    {
    case cgltf_filter_type_linear:
    case cgltf_filter_type_linear_mipmap_linear:
        create_info.minFilter = VK_FILTER_LINEAR;
        break;

    case cgltf_filter_type_nearest:
    case cgltf_filter_type_nearest_mipmap_linear:
        create_info.minFilter = VK_FILTER_NEAREST;
        break;

    default:
        break;
    }

    switch (mag_filter)
    {
    case cgltf_filter_type_linear:
    case cgltf_filter_type_linear_mipmap_linear:
        create_info.magFilter = VK_FILTER_LINEAR;
        break;

    case cgltf_filter_type_nearest:
    case cgltf_filter_type_nearest_mipmap_linear:
        create_info.magFilter = VK_FILTER_NEAREST;
        break;

    default:
        break;
    }

    switch (wrap_s)
    {
    case cgltf_wrap_mode_clamp_to_edge:
        create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        break;

    case cgltf_wrap_mode_repeat:
        create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        break;

    case cgltf_wrap_mode_mirrored_repeat:
        create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        break;

    default:
        break;
    }

    switch (wrap_s)
    {
    case cgltf_wrap_mode_clamp_to_edge:
        create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        break;

    case cgltf_wrap_mode_repeat:
        create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        break;

    case cgltf_wrap_mode_mirrored_repeat:
        create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        break;

    default:
        break;
    }

    VkSampler sampler = VK_NULL_HANDLE;

    VK_CHECK("create sampler", vkCreateSampler(device, &create_info, nullptr, &sampler));

#ifdef DEBUG
    VkDebugUtilsObjectNameInfoEXT name_info = {};
    name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    name_info.objectType = VK_OBJECT_TYPE_SAMPLER;
    name_info.objectHandle = reinterpret_cast<uint64_t>(sampler);
    name_info.pObjectName = name.c_str();

    VK_CHECK("setting sampler name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // DEBUG

    return sampler;
}

void vk_sampler::destroy(const VkSampler sampler, const VkDevice device)
{
    if (sampler != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroySampler(device, sampler, nullptr);
    }
}

vk_desc_img_info::data vk_desc_img_info::create(const VkDevice device)
{
    data d = {};
    d.image = VK_NULL_HANDLE;

    return d;
}

void vk_desc_img_info::destroy(const vk_desc_img_info::data image_info, const VkDevice device)
{
    if (device != VK_NULL_HANDLE)
    {
        vk_image::destroy(image_info.image, device);
        vk_image_view::destroy(image_info.desc.imageView, device);
        vk_sampler::destroy(image_info.desc.sampler, device);
    }
}
