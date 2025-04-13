#include "vk_renderer.hpp"

#include <iostream>

inline static VkViewport RECT_TO_VIEWPORT(const RECT& rect)
{
	const VkViewport v = {
		.x = 0,
		.y = 0,
		.width = static_cast<float>(rect.right - rect.left),
		.height = static_cast<float>(rect.bottom - rect.top),
		.minDepth = 0,
		.maxDepth = 1,
	};

	return v;
}

inline static RECT VIEWPORT_TO_RECT(const VkViewport& viewport)
{
	const RECT r = {
		.left = static_cast<LONG>(viewport.x),
		.top = static_cast<LONG>(viewport.y),
		.right = static_cast<LONG>(viewport.width),
		.bottom = static_cast<LONG>(viewport.height),
	};

	return r;
}

inline static RECT SANITIZE_RECT_FOR_RENDER(const RECT& rect)
{
	const RECT r = {
		.left = 0,
		.top = 0,
		.right = rect.right - rect.left,
		.bottom = rect.bottom - rect.top,
	};

	return r;
}

vk_renderer::vk_renderer(const HWND h_wnd) : img_idx(0), acq_wait_sem_val(0)
{
	VkResult result = volkInitialize();
	instance = std::make_unique<vk_instance>();
	volkLoadInstance(instance->instance);
	surface = std::make_unique<vk_surface>(instance->instance, GetModuleHandleA(nullptr), h_wnd);
	phy_dev = std::make_unique<vk_phydev>(instance->instance, surface.get());
	device = std::make_unique<vk_device>(phy_dev->phy_dev, phy_dev->q_fly_idx, phy_dev->q_count);
	swapchain = std::make_unique<vk_swapchain>(device->device, surface.get(), phy_dev.get());
	acq_sig_sem = std::make_unique<vk_semaphore>(device->device, false);
	acq_wait_sem = std::make_unique<vk_semaphore>(device->device, true);

	const VkSemaphoreSignalInfo sem_sig_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
		.semaphore = acq_wait_sem->semaphore,
		.value = ++acq_wait_sem_val,
	};
	vkSignalSemaphore(device->device, &sem_sig_info);

	GetWindowRect(h_wnd, &wnd_rect);
	wnd_rect = SANITIZE_RECT_FOR_RENDER(wnd_rect);
	viewport = RECT_TO_VIEWPORT(wnd_rect);
}

void vk_renderer::import_scene_data(const cgltf_data* data)
{
}

void vk_renderer::resize(const UINT width, const UINT height)
{
}

void vk_renderer::begin_frame()
{
	uint64_t wait_values = acq_wait_sem_val;

	const VkSemaphoreWaitInfo wait_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		.semaphoreCount = 1,
		.pSemaphores = &acq_wait_sem->semaphore,
		.pValues = &wait_values,
	};
	vkWaitSemaphores(device->device, &wait_info, UINT64_MAX);

	VK_CHECK("acquire image index", vkAcquireNextImageKHR(device->device, swapchain->swapchain, UINT64_MAX, acq_sig_sem->semaphore, VK_NULL_HANDLE, &img_idx));
	VK_CHECK("reset command pool", vkResetCommandPool(device->device, swapchain->cmd_pools[img_idx], VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));

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
		.image = swapchain->images[img_idx],
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

	vkBeginCommandBuffer(swapchain->cmd_buffs[img_idx], &begin_info);
	vkCmdPipelineBarrier2(swapchain->cmd_buffs[img_idx], &dep_info);
}

void vk_renderer::clear_frame(const float color[])
{
	const VkRenderingAttachmentInfoKHR color_attachment_infos[] = {
	{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = swapchain->image_views[img_idx],
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = {
			.color = {
				.float32 = {
					color[0], 1, color[2], color[3]
				},
			},
		},
	},
	};

	const VkRenderingInfoKHR rendering_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = {
			.extent = surface->surf_caps.currentExtent,
		},
		.layerCount = 1,
		.colorAttachmentCount = _countof(color_attachment_infos),
		.pColorAttachments = color_attachment_infos,
	};

	vkCmdBeginRendering(swapchain->cmd_buffs[img_idx], &rendering_info);
}

void vk_renderer::render_world()
{
}

void vk_renderer::end_frame()
{
	vkCmdEndRendering(swapchain->cmd_buffs[img_idx]);

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
		.image = swapchain->images[img_idx],
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

	vkCmdPipelineBarrier2(swapchain->cmd_buffs[img_idx], &dep_info);

	vkEndCommandBuffer(swapchain->cmd_buffs[img_idx]);

	const VkDeviceQueueInfo2 q_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
		.queueFamilyIndex = phy_dev->q_fly_idx,
		.queueIndex = 0,
	};
	VkQueue q = VK_NULL_HANDLE;

	vkGetDeviceQueue2(device->device, &q_info, &q);

	const VkSemaphoreSubmitInfo wait_sem_infos[] =
	{
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = acq_sig_sem->semaphore,
			.stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		}
	};

	const VkSemaphoreSubmitInfo sig_sem_infos[] =
	{
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = swapchain->rndr_semaphores[img_idx],
			.stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		},
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = acq_wait_sem->semaphore,
			.value = ++acq_wait_sem_val,
			.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		}
	};

	const VkCommandBufferSubmitInfo cmd_buff_infos[] =
	{
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = swapchain->cmd_buffs[img_idx],
		}
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

	vkQueueSubmit2(q, 1, &submit_info, VK_NULL_HANDLE);

	const VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &swapchain->rndr_semaphores[img_idx],
		.swapchainCount = 1,
		.pSwapchains = &swapchain->swapchain,
		.pImageIndices = &img_idx,
	};

	vkQueuePresentKHR(q, &present_info);
}

void vk_renderer::clear_scene_data()
{
}

vk_renderer::~vk_renderer()
{
	vkDeviceWaitIdle(device->device);
}
