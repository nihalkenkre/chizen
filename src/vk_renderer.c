#include "vk_renderer.h"
#include "vk_objects.h"

VkInstance instance = VK_NULL_HANDLE;
surface_data surface = { 0 };
phy_dev_data phy_dev = { 0 };
VkDevice device = VK_NULL_HANDLE;
vk_swapchain_data swapchain = { 0 };
vk_cmd_pool_data xfer_cmd_pool = { 0 };
vk_semaphore_data acq_sig_sem = { 0 };
vk_semaphore_data acq_wait_sem = { 0 };

uint64_t acq_wait_sem_val = 0;

RECT wnd_rect = { 0 };
VkViewport viewport = { 0 };

VkQueue gfx_q = VK_NULL_HANDLE;
VkQueue xfer_q = VK_NULL_HANDLE;

uint32_t img_idx = 0;

inline static VkViewport RECT_TO_VIEWPORT(const RECT rect)
{
    const VkViewport v = {
        .x = 0,
        .y = 0,
        .width = (float)(rect.right - rect.left),
        .height = (float)(rect.bottom - rect.top),
        .minDepth = 0,
        .maxDepth = 1,
    };

    return v;
}

inline static RECT VIEWPORT_TO_RECT(const VkViewport viewport)
{
    const RECT r = {
        .left = (LONG)(viewport.x),
        .top = (LONG)(viewport.y),
        .right = (LONG)(viewport.width),
        .bottom = (LONG)(viewport.height),
    };

    return r;
}

inline static RECT SANITIZE_RECT_FOR_RENDER(const RECT rect)
{
    const RECT r = {
        .left = 0,
        .top = 0,
        .right = rect.right - rect.left,
        .bottom = rect.bottom - rect.top,
    };

    return r;
}

void vk_renderer_init(renderer* r, const HWND h_wnd)
{
    instance = vk_instance_create();
    surface = vk_surface_create(instance, GetModuleHandleA(NULL), h_wnd);
    phy_dev = vk_get_phy_dev_s(instance, &surface);
    device = vk_device_create(phy_dev.phy_dev, phy_dev.q_fly_idx, phy_dev.q_count);
    swapchain = vk_swapchain_create(device, &surface, &phy_dev, "swapchain");
    xfer_cmd_pool = vk_command_pool_create(device, phy_dev.q_fly_idx, 1, "xfer cmd pool");
    acq_sig_sem = vk_semaphore_create(device, VK_SEMAPHORE_TYPE_BINARY, "acq sig sem");
    acq_wait_sem = vk_semaphore_create(device, VK_SEMAPHORE_TYPE_TIMELINE, "acq wait sem");

    // signalling so that the render function does not stall on vkAcquireNextImage the first time
    const VkSemaphoreSignalInfo sem_sig_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
        .semaphore = acq_wait_sem.semaphore,
        .value = ++acq_wait_sem_val,
    };
    VK_CHECK("signal acq wait sem", vkSignalSemaphore(device, &sem_sig_info));

    GetWindowRect(h_wnd, &r->wnd_rect);
    wnd_rect = SANITIZE_RECT_FOR_RENDER(wnd_rect);
    viewport = RECT_TO_VIEWPORT(wnd_rect);

    VkDeviceQueueInfo2 queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
        .queueFamilyIndex = phy_dev.q_fly_idx,
        .queueIndex = 0,
    };
    vkGetDeviceQueue2(device, &queue_info, &gfx_q);

    queue_info.queueIndex = 1;
    vkGetDeviceQueue2(device, &queue_info, &xfer_q);

    r->import_scene_data = vk_renderer_import_scene_data;
    r->resize = vk_renderer_resize;
    r->begin_frame = vk_renderer_begin_frame;
    r->clear_frame = vk_renderer_clear_frame;
    r->render_world = vk_renderer_render_world;
    r->end_frame = vk_renderer_end_frame;
    r->clear_scene_data = vk_renderer_clear_scene_data;
    r->render_offline = vk_renderer_render_offline;
    r->shutdown = vk_renderer_shutdown;
}

void vk_renderer_import_scene_data(const char* file_path)
{
}

void vk_renderer_resize(const uint32_t width, const uint32_t height)
{
    VK_CHECK("wait for present fence", vkWaitForFences(device, 1, &swapchain.present_fences[img_idx], VK_TRUE, UINT64_MAX));
    VK_CHECK("reset present fence", vkResetFences(device, 1, &swapchain.present_fences[img_idx]));

    VK_CHECK("get surface capabilities", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phy_dev.phy_dev, surface.surface, &surface.surf_caps));

    vk_swapchain_destroy(swapchain, device);
    swapchain = vk_swapchain_create(device, &surface, &phy_dev, "swapchain");
}

void vk_renderer_begin_frame(void)
{
    const VkSemaphoreWaitInfo wait_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores = &acq_wait_sem.semaphore,
        .pValues = &acq_wait_sem_val,
    };
    VK_CHECK("wait for acq wait sem", vkWaitSemaphores(device, &wait_info, UINT64_MAX));

    VK_CHECK("acquire image index", vkAcquireNextImageKHR(device, swapchain.swapchain, UINT64_MAX, acq_sig_sem.semaphore, VK_NULL_HANDLE, &img_idx));
    VK_CHECK("reset command buffer", vkResetCommandBuffer(swapchain.cmd_buffs[img_idx], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

    const VkImageMemoryBarrier2 img_mem_barr2 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = 0,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain.images[img_idx],
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };

    const VkDependencyInfo dep_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &img_mem_barr2,
    };

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    VK_CHECK("begin command buffer", vkBeginCommandBuffer(swapchain.cmd_buffs[img_idx], &begin_info));
    vkCmdPipelineBarrier2(swapchain.cmd_buffs[img_idx], &dep_info);
}

void vk_renderer_clear_frame(const float color[])
{
    const VkRenderingAttachmentInfoKHR color_attachment_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = swapchain.image_views[img_idx],
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {
                .color = {
                    .float32 = {
                        color[0], color[1], color[2], color[3]
                    },
                },
            },
        },
    };
    //VkRenderingAttachmentInfo depth_attachment_info = {};
    //depth_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    //depth_attachment_info.imageView = sd.depth_texture_view;
    //depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    //depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    //depth_attachment_info.clearValue.depthStencil.depth = 1.f;
    //depth_attachment_info.clearValue.depthStencil.stencil = 0;

    const VkRenderingInfoKHR rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea.extent = surface.surf_caps.currentExtent,
        .layerCount = 1,
        .colorAttachmentCount = _countof(color_attachment_infos),
        .pColorAttachments = color_attachment_infos,
        //rendering_info.pDepthAttachment = &depth_attachment_info;
    };

    vkCmdBeginRendering(swapchain.cmd_buffs[img_idx], &rendering_info);
}

void vk_renderer_render_world(const mat4 cam_xform)
{
}

void vk_renderer_end_frame(void)
{
    vkCmdEndRendering(swapchain.cmd_buffs[img_idx]);

    const VkImageMemoryBarrier2 img_mem_barr = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain.images[img_idx],
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.levelCount = 1,
        .subresourceRange.layerCount = 1,
    };

    const VkDependencyInfo dep_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &img_mem_barr,
    };

    vkCmdPipelineBarrier2(swapchain.cmd_buffs[img_idx], &dep_info);

    VK_CHECK("end buffer", vkEndCommandBuffer(swapchain.cmd_buffs[img_idx]));

    const VkSemaphoreSubmitInfo wait_sem_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = acq_sig_sem.semaphore,
            .stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        }
    };

    const VkSemaphoreSubmitInfo sig_sem_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = swapchain.rndr_semaphores[img_idx],
            .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = acq_wait_sem.semaphore,
            .value = ++acq_wait_sem_val,
            .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        },
    };

    const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = swapchain.cmd_buffs[img_idx],
        },
    };

    const VkSubmitInfo2 submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = _countof(wait_sem_infos),
        .pWaitSemaphoreInfos = wait_sem_infos,
        .commandBufferInfoCount = _countof(cmd_buff_infos),
        .pCommandBufferInfos = cmd_buff_infos,
        .signalSemaphoreInfoCount = _countof(sig_sem_infos),
        .pSignalSemaphoreInfos = sig_sem_infos,
    };

    VK_CHECK("queue submit", vkQueueSubmit2(gfx_q, 1, &submit_info, VK_NULL_HANDLE));

    const VkSwapchainPresentFenceInfoEXT present_fence_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT,
        .swapchainCount = 1,
        .pFences = &swapchain.present_fences[img_idx],
    };

    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = &present_fence_info,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &swapchain.rndr_semaphores[img_idx],
        .swapchainCount = 1,
        .pSwapchains = &swapchain.swapchain,
        .pImageIndices = &img_idx,
    };

    VK_CHECK("reset present fence", vkResetFences(device, 1, &swapchain.present_fences[img_idx]));
    VK_CHECK("queue present", vkQueuePresentKHR(gfx_q, &present_info));
}

void vk_renderer_clear_scene_data(void)
{
}

void vk_renderer_render_offline(const uint32_t render_width, const uint32_t render_height, uint8_t* pixels)
{
}

void vk_renderer_shutdown(void)
{
    VK_CHECK("wait for present fence", vkWaitForFences(device, 1, &swapchain.present_fences[img_idx], VK_TRUE, UINT64_MAX));
    VK_CHECK("reset present fence", vkResetFences(device, 1, &swapchain.present_fences[img_idx]));

    vk_semaphore_destroy(acq_sig_sem.semaphore, device);
    vk_semaphore_destroy(acq_wait_sem.semaphore, device);
    vk_command_pool_destroy(xfer_cmd_pool, device);
    vk_swapchain_destroy(swapchain, device);
    vk_device_destroy(device);
    vk_surface_destroy(surface.surface, instance);
    vk_instance_destroy(instance);
}
