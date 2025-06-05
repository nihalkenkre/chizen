#include "vk_objects.h"
#include <stdbool.h>

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(
    VkDevice                                    device,
    const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
    return vk_SetDebugUtilsObjectNameEXT(device, pNameInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesKHR(
    VkDevice                                    device,
    VkDeferredOperationKHR                      deferredOperation,
    VkPipelineCache                             pipelineCache,
    uint32_t                                    createInfoCount,
    const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines)
{
    return vk_CreateRayTracingPipelinesKHR(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureBuildSizesKHR(
    VkDevice                                    device,
    VkAccelerationStructureBuildTypeKHR         buildType,
    const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
    const uint32_t* pMaxPrimitiveCounts,
    VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo)
{
    vk_GetAccelerationStructureBuildSizesKHR(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureKHR(
    VkDevice                                    device,
    const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkAccelerationStructureKHR* pAccelerationStructure)
{
    return vk_CreateAccelerationStructureKHR(device, pCreateInfo, pAllocator, pAccelerationStructure);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureKHR(
    VkDevice                                    device,
    VkAccelerationStructureKHR                  accelerationStructure,
    const VkAllocationCallbacks* pAllocator)
{
    vk_DestroyAccelerationStructureKHR(device, accelerationStructure, pAllocator);
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

void copy_buffer_to_buffer(const VkBuffer src_buffer, const VkBuffer dst_buffer, const VkBufferCopy2* regions, const uint32_t regions_count, const VkCommandBuffer cmd_buff, const VkQueue xfer_q)
{
    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_CHECK("begin command buffer", vkBeginCommandBuffer(cmd_buff, &begin_info));

    const VkCopyBufferInfo2 copy_info = {
        .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
        .srcBuffer = src_buffer,
        .dstBuffer = dst_buffer,
        .regionCount = regions_count,
        .pRegions = regions,
    };
    vkCmdCopyBuffer2(cmd_buff, &copy_info);

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

void copy_buffer_to_image(const VkBuffer src_buffer, const VkImage dst_image, const VkImageLayout dst_image_layout, const VkBufferImageCopy2* regions, const uint32_t regions_count, const VkCommandBuffer cmd_buff, const VkQueue xfer_q)
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
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.levelCount = 1,
        .subresourceRange.layerCount = 1,
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
        .regionCount = regions_count,
        .pRegions = regions,
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
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.levelCount = 1,
        .subresourceRange.layerCount = 1,
    };

    dependency_info.pImageMemoryBarriers = &img_lyt_chng_bar;
    vkCmdPipelineBarrier2(cmd_buff, &dependency_info);

    VK_CHECK("end command buffer", vkEndCommandBuffer(cmd_buff));

    VkSubmitInfo submit_infos[] = {
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

VkInstance vk_instance_create(void)
{
    VkInstance instance = VK_NULL_HANDLE;

    const char* req_ext_names[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME,
        VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
#ifdef DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

    uint32_t prop_count = 0;
    VK_CHECK("enumerate instance extensions", vkEnumerateInstanceExtensionProperties(NULL, &prop_count, NULL));

    VkExtensionProperties* props = malloc(sizeof(VkExtensionProperties) * prop_count);
    VK_CHECK("enumerate instance extensions", vkEnumerateInstanceExtensionProperties(NULL, &prop_count, props));

    for (uint32_t e = 0; e < _countof(req_ext_names); ++e)
    {
        bool ext_found = false;
        for (uint32_t p = 0; p < prop_count; ++p)
        {
            if (strcmp(props[p].extensionName, req_ext_names[e]) == 0)
            {
                ext_found = true;
                break;
            }
        }

        if (!ext_found)
        {
            printf("%s not found.\nExiting...\n", req_ext_names[e]);
            goto shutdown;
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
        .enabledExtensionCount = _countof(req_ext_names),
        .ppEnabledExtensionNames = req_ext_names,
    };

    VK_CHECK("create instance", vkCreateInstance(&create_info, NULL, &instance));

shutdown:

    free(props);
    return instance;
}

void vk_instance_destroy(VkInstance instance)
{
    if (instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, NULL);
        instance = VK_NULL_HANDLE;
    }
}

surface_data vk_surface_create(const VkInstance instance, const HINSTANCE h_instance, const HWND h_wnd)
{
    surface_data s = { 0 };

    const VkWin32SurfaceCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = h_instance,
        .hwnd = h_wnd,
    };

    VK_CHECK("create surface", vkCreateWin32SurfaceKHR(instance, &create_info, NULL, &s.surface));

    s.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    s.h_instance = h_instance;
    s.h_wnd = h_wnd;

    return s;
}

void vk_surface_destroy(VkSurfaceKHR surface, const VkInstance instance)
{
    if (surface != VK_NULL_HANDLE && instance != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance, surface, NULL);
        surface = VK_NULL_HANDLE;
    }
}

phy_dev_data vk_get_phy_dev_s(const VkInstance instance, surface_data* s)
{
    phy_dev_data d = {
        .props = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        },
        .mem_props = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
        },
    };

    uint32_t phy_dev_count = 0;
    VK_CHECK("enumerate physical devices", vkEnumeratePhysicalDevices(instance, &phy_dev_count, NULL));

    VkPhysicalDevice* phy_devs = malloc(sizeof(VkPhysicalDevice) * phy_dev_count);
    VK_CHECK("enumerate physical devices", vkEnumeratePhysicalDevices(instance, &phy_dev_count, phy_devs));

    for (uint32_t pd = 0; pd < phy_dev_count; ++pd)
    {
        vkGetPhysicalDeviceProperties2(phy_devs[pd], &d.props);

        if (d.props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            uint32_t q_fly_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(phy_devs[pd], &q_fly_count, NULL);

            VkQueueFamilyProperties* q_fly_props = malloc(sizeof(VkQueueFamilyProperties) * q_fly_count);
            vkGetPhysicalDeviceQueueFamilyProperties(phy_devs[pd], &q_fly_count, q_fly_props);

            for (uint32_t q = 0; q < q_fly_count; ++q)
            {
                VkBool32 is_supported = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(phy_devs[pd], q, s->surface, &is_supported);

                if (is_supported &&
                    vkGetPhysicalDeviceWin32PresentationSupportKHR(phy_devs[pd], q) &&
                    q_fly_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    d.phy_dev = phy_devs[pd];
                    d.q_count = q_fly_props[q].queueCount;
                    d.q_fly_idx = q;

                    vkGetPhysicalDeviceMemoryProperties2(phy_devs[pd], &d.mem_props);

                    VK_CHECK("get surface caps", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phy_devs[pd], s->surface, &s->surf_caps));

                    {
                        uint32_t surf_forms_count = 0;
                        VK_CHECK("get surface formats", vkGetPhysicalDeviceSurfaceFormatsKHR(phy_devs[pd], s->surface, &surf_forms_count, NULL));

                        VkSurfaceFormatKHR* surf_forms = malloc(sizeof(VkSurfaceFormatKHR) * surf_forms_count);
                        VK_CHECK("get surface formats", vkGetPhysicalDeviceSurfaceFormatsKHR(phy_devs[pd], s->surface, &surf_forms_count, surf_forms));

                        for (uint32_t sf = 0; sf < surf_forms_count; ++sf)
                        {
                            if ((surf_forms[sf].format == VK_FORMAT_R8G8B8A8_UNORM) && (surf_forms[sf].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
                            {
                                s->format = surf_forms[sf];
                                break;
                            }
                        }

                        free(surf_forms);
                    }

                    {
                        uint32_t present_modes_count = 0;
                        VK_CHECK("get present modes", vkGetPhysicalDeviceSurfacePresentModesKHR(phy_devs[pd], s->surface, &present_modes_count, NULL));

                        VkPresentModeKHR* present_modes = malloc(sizeof(VkPresentModeKHR) * present_modes_count);
                        VK_CHECK("get surface formats", vkGetPhysicalDeviceSurfacePresentModesKHR(phy_devs[pd], s->surface, &present_modes_count, present_modes));

                        for (uint32_t pm = 0; pm < present_modes_count; ++pm)
                        {
                            if (present_modes[pm] == VK_PRESENT_MODE_MAILBOX_KHR)
                            {
                                s->present_mode = present_modes[pm];
                                break;
                            }
                        }

                        free(present_modes);
                    }
                    break;
                }
            }

            free(q_fly_props);
        }
    }

    free(phy_devs);

    return d;
}

phy_dev_data vk_get_phy_dev(const VkInstance instance)
{
    phy_dev_data d = {
        .props = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        },
        .mem_props = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
        },
    };

    uint32_t phy_dev_count = 0;
    VK_CHECK("enumerate physical devices", vkEnumeratePhysicalDevices(instance, &phy_dev_count, NULL));

    VkPhysicalDevice* phy_devs = malloc(sizeof(VkPhysicalDevice) * phy_dev_count);
    VK_CHECK("enumerate physical devices", vkEnumeratePhysicalDevices(instance, &phy_dev_count, phy_devs));

    for (uint32_t pd = 0; pd < phy_dev_count; ++pd)
    {
        vkGetPhysicalDeviceProperties2(phy_devs[pd], &d.props);

        if (d.props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            uint32_t q_fly_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(phy_devs[pd], &q_fly_count, NULL);

            VkQueueFamilyProperties* q_fly_props = malloc(sizeof(VkQueueFamilyProperties) * q_fly_count);
            vkGetPhysicalDeviceQueueFamilyProperties(phy_devs[pd], &q_fly_count, q_fly_props);

            for (uint32_t q = 0; q < q_fly_count; ++q)
            {
                if (q_fly_props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    d.phy_dev = phy_devs[pd];
                    d.q_count = q_fly_props[q].queueCount;
                    d.q_fly_idx = q;

                    vkGetPhysicalDeviceMemoryProperties2(phy_devs[pd], &d.mem_props);

                    break;
                }
            }

            free(q_fly_props);
        }
    }

    free(phy_devs);

    return d;

}

VkDevice vk_device_create(const VkPhysicalDevice phy_dev, const uint32_t q_fly_idx, const uint32_t q_count)
{
    VkDevice device = VK_NULL_HANDLE;
    float* priorities = malloc(sizeof(float) * q_count);

    for (uint32_t q = 0; q < q_count; ++q)
    {
        priorities[q] = 1.f;
    }

    const char* req_ext_names[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
        VK_EXT_MESH_SHADER_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
        VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    };

    uint32_t prop_count = 0;
    VK_CHECK("enumerate dev ext props", vkEnumerateDeviceExtensionProperties(phy_dev, NULL, &prop_count, NULL));

    VkExtensionProperties* props = malloc(sizeof(VkExtensionProperties) * prop_count);
    VK_CHECK("enumerate dev ext props", vkEnumerateDeviceExtensionProperties(phy_dev, NULL, &prop_count, props));

    for (uint32_t e = 0; e < _countof(req_ext_names); ++e)
    {
        bool ext_found = false;
        for (uint32_t p = 0; p < prop_count; ++p)
        {
            if (strcmp(props[p].extensionName, req_ext_names[e]) == 0)
            {
                ext_found = true;
                break;
            }
        }

        if (!ext_found)
        {
            printf("%s not found.\nExiting...\n", req_ext_names[e]);
            goto shutdown;
        }
    }


    const VkDeviceQueueCreateInfo q_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = q_fly_idx,
        .queueCount = q_count,
        .pQueuePriorities = priorities,
    };

    VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchain_main_1_feats = {
       .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT,
    };

    VkPhysicalDeviceAccelerationStructureFeaturesKHR acc_str_feats = {
       .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
       .pNext = &swapchain_main_1_feats,
    };

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipe_feats = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .pNext = &acc_str_feats,
    };

    VkPhysicalDeviceRobustness2FeaturesEXT rob2_feats = {
       .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
       .pNext = &rt_pipe_feats,
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures bda_feats = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .pNext = &rob2_feats,
    };

    VkPhysicalDeviceMaintenance4Features main_4_feats = {
       .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES,
       .pNext = &bda_feats,
    };

    VkPhysicalDeviceDynamicRenderingFeatures dyn_rend_feats = {
       .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
       .pNext = &bda_feats,
    };

    VkPhysicalDeviceSynchronization2Features sync2_feats = {
       .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
       .pNext = &dyn_rend_feats,
    };

    VkPhysicalDeviceTimelineSemaphoreFeatures time_sem_feats = {
       .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
       .pNext = &sync2_feats,
    };

    VkPhysicalDeviceFeatures2 feats2 = {};
    feats2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    feats2.pNext = &time_sem_feats;

    vkGetPhysicalDeviceFeatures2(phy_dev, &feats2);

    rt_pipe_feats.rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE;
    rt_pipe_feats.rayTracingPipelineShaderGroupHandleCaptureReplayMixed = VK_FALSE;
    rt_pipe_feats.rayTracingPipelineTraceRaysIndirect = VK_FALSE;
    acc_str_feats.accelerationStructureCaptureReplay = VK_FALSE;

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pNext = &feats2;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &q_create_info;
    create_info.enabledExtensionCount = _countof(req_ext_names);
    create_info.ppEnabledExtensionNames = req_ext_names;

    VK_CHECK("create device", vkCreateDevice(phy_dev, &create_info, NULL, &device));

    vk_SetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)(vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
    vk_CreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
    vk_GetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
    vk_CreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
    vk_DestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));

shutdown:
    free(priorities);
    free(props);
    return device;
}

void vk_device_destroy(VkDevice device)
{
    if (device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device, NULL);
        device = VK_NULL_HANDLE;
    }
}

vk_swapchain_data vk_swapchain_create(const VkDevice device, const surface_data* sd, const phy_dev_data* pd, const char* name)
{
    const VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = sd->surface,
        .minImageCount = sd->surf_caps.minImageCount,
        .imageFormat = sd->format.format,
        .imageColorSpace = sd->format.colorSpace,
        .imageExtent = sd->surf_caps.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = sd->surf_caps.supportedUsageFlags,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &pd->q_fly_idx,
        .preTransform = sd->surf_caps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = sd->present_mode,
    };

    vk_swapchain_data d = { 0 };
    VK_CHECK("create swapchain", vkCreateSwapchainKHR(device, &create_info, NULL, &d.swapchain));

    VK_CHECK("get swapchain images", vkGetSwapchainImagesKHR(device, d.swapchain, &d.images_count, NULL));
    d.images = malloc(sizeof(VkImage) * d.images_count);
    VK_CHECK("get swapchain images", vkGetSwapchainImagesKHR(device, d.swapchain, &d.images_count, d.images));

    d.image_views = malloc(sizeof(VkImageView) * d.images_count);
    d.cmd_buffs = malloc(sizeof(VkCommandBuffer) * d.images_count);
    d.rndr_semaphores = malloc(sizeof(VkSemaphore) * d.images_count);
    d.present_fences = malloc(sizeof(VkFence) * d.images_count);

    VkImageViewCreateInfo image_view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = sd->format.format,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.levelCount = 1,
        .subresourceRange.layerCount = 1,
    };

    const VkCommandPoolCreateInfo cmd_pool_ci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = pd->q_fly_idx,
    };
    VK_CHECK("create command pool", vkCreateCommandPool(device, &cmd_pool_ci, NULL, &d.cmd_pool));

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
        VK_CHECK("create swapchain image view", vkCreateImageView(device, &image_view_create_info, NULL, &d.image_views[i]));

        cmd_buff_ai.commandPool = d.cmd_pool;
        VK_CHECK("allocate command buffer", vkAllocateCommandBuffers(device, &cmd_buff_ai, &d.cmd_buffs[i]));
        VK_CHECK("create semaphore", vkCreateSemaphore(device, &sem_ci, NULL, &d.rndr_semaphores[i]));
        VK_CHECK("create fence", vkCreateFence(device, &fence_ci, NULL, &d.present_fences[i]));
    }

#ifdef DEBUG
    VkDebugUtilsObjectNameInfoEXT name_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_SWAPCHAIN_KHR,
        .objectHandle = (uint64_t)(d.swapchain),
        .pObjectName = name,
    };
    VK_CHECK("set swapchain name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

    name_info.objectType = VK_OBJECT_TYPE_COMMAND_POOL;
    name_info.objectHandle = (uint64_t)(d.cmd_pool);
    name_info.pObjectName = "swapchain command pool";

    for (uint32_t i = 0; i < d.images_count; ++i)
    {
        char i_str[16];
        char object_name[128] = "swapchain image ";

        name_info.objectType = VK_OBJECT_TYPE_IMAGE;
        name_info.objectHandle = (uint64_t)(d.images[i]);
        name_info.pObjectName = strcat(object_name, _itoa(i, i_str, 10));
        VK_CHECK("setting swapchain image name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

        strcpy(object_name, "swapchain image view ");
        name_info.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
        name_info.objectHandle = (uint64_t)(d.image_views[i]);
        name_info.pObjectName = strcat(object_name, _itoa(i, i_str, 10));
        VK_CHECK("setting swapcahin image view name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

        strcpy(object_name, "swapchain command buffer ");
        name_info.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
        name_info.objectHandle = (uint64_t)(d.cmd_buffs[i]);
        name_info.pObjectName = strcat(object_name, _itoa(i, i_str, 10));
        VK_CHECK("setting swapchain command buffer name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

        strcpy(object_name, "swapchain render semaphore ");
        name_info.objectType = VK_OBJECT_TYPE_SEMAPHORE;
        name_info.objectHandle = (uint64_t)(d.rndr_semaphores[i]);
        name_info.pObjectName = strcat(object_name, _itoa(i, i_str, 10));
        VK_CHECK("setting swapchain render sem name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

        strcpy(object_name, "swapchain present fence ");
        name_info.objectType = VK_OBJECT_TYPE_FENCE;
        name_info.objectHandle = (uint64_t)(d.present_fences[i]);
        name_info.pObjectName = strcat(object_name, _itoa(i, i_str, 10));
        VK_CHECK("setting swapchain present fence name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
    }
#endif // DEBUG

    return d;
}

void vk_swapchain_destroy(vk_swapchain_data sd, const VkDevice device)
{
    if (sd.cmd_pool != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, sd.cmd_pool, NULL);
        sd.cmd_pool = VK_NULL_HANDLE;
    }

    for (uint32_t i = 0; i < sd.images_count; ++i)
    {
        vkDestroyFence(device, sd.present_fences[i], NULL);
        sd.present_fences[i] = VK_NULL_HANDLE;
        vkDestroyImageView(device, sd.image_views[i], NULL);
        sd.image_views[i] = VK_NULL_HANDLE;
        vkDestroySemaphore(device, sd.rndr_semaphores[i], NULL);
        sd.rndr_semaphores[i] = VK_NULL_HANDLE;
    }

    free(sd.present_fences);
    free(sd.image_views);
    free(sd.rndr_semaphores);

    if (sd.swapchain != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device, sd.swapchain, NULL);
        sd.swapchain = VK_NULL_HANDLE;
    }
}

vk_cmd_pool_data vk_command_pool_create(const VkDevice device, const uint32_t q_fly_idx, const uint32_t cmd_buffs_count, const char* name)
{
    vk_cmd_pool_data d = { 0 };

    const VkCommandPoolCreateInfo cmd_pool_ci = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = q_fly_idx,
    };

    VK_CHECK("create command pool", vkCreateCommandPool(device, &cmd_pool_ci, NULL, &d.cmd_pool));

    const VkCommandBufferAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = d.cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = cmd_buffs_count,
    };

    d.cmd_buffs = malloc(sizeof(VkCommandBuffer) * cmd_buffs_count);
    d.cmd_buffs_count = cmd_buffs_count;

    VK_CHECK("allocate command buffer", vkAllocateCommandBuffers(device, &allocate_info, d.cmd_buffs));

#ifdef DEBUG
    const VkDebugUtilsObjectNameInfoEXT name_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_COMMAND_POOL,
        .objectHandle = (uint64_t)(d.cmd_pool),
        .pObjectName = name,
    };

    VK_CHECK("setting command pool name", vkSetDebugUtilsObjectNameEXT(device, &name_info));

    for (size_t cb = 0; cb < d.cmd_buffs_count; ++cb)
    {
        char itoa[2] = {};
        char c_name[1024];
        strcpy(c_name, name);
        strcat(c_name, " commmad buffer ");
        strcat(c_name, _itoa((int)(cb), itoa, 10));

        const VkDebugUtilsObjectNameInfoEXT name_info = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType = VK_OBJECT_TYPE_COMMAND_BUFFER,
            .objectHandle = (uint64_t)(d.cmd_buffs[cb]),
            .pObjectName = c_name,
        };

        VK_CHECK("setting command pool name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
    }
#endif // DEBUG

    return d;
}

void vk_command_pool_destroy(vk_cmd_pool_data cd, const VkDevice device)
{
    if (cd.cmd_pool != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, cd.cmd_pool, NULL);
        cd.cmd_pool = VK_NULL_HANDLE;
    }
}

vk_semaphore_data vk_semaphore_create(const VkDevice device, const VkSemaphoreType semaphore_type, const char* name)
{
    const VkSemaphoreTypeCreateInfo t_ci = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .initialValue = 0,
        .semaphoreType = semaphore_type,
    };

    const VkSemaphoreCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &t_ci,
    };

    vk_semaphore_data d = {};
    d.type = semaphore_type;

    VK_CHECK("create semaphore", vkCreateSemaphore(device, &create_info, NULL, &d.semaphore));

#ifdef DEBUG
    const VkDebugUtilsObjectNameInfoEXT name_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = VK_OBJECT_TYPE_SEMAPHORE,
        .objectHandle = (uint64_t)(d.semaphore),
        .pObjectName = name,
    };
    VK_CHECK("set semaphore name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
#endif // DEBUG

    return d;
}

void vk_semaphore_destroy(VkSemaphore semaphore, const VkDevice device)
{
    if (semaphore != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(device, semaphore, NULL);
        semaphore = VK_NULL_HANDLE;
    }
}
