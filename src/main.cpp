#include "vk_objects.hpp"

struct VulkanState {
	VkInstance instance = VK_NULL_HANDLE;
	vk_surface::data surface_data = {};
	vk_phydev::data phy_dev_data = {};
	vk_device::data device_data = {};
	vk_swapchain::data swapchain_data = {};
	vk_command_pool::data xfer_cmd_pool_data = {};
	VkFence fence = VK_NULL_HANDLE;
	vk_semaphore::data acq_sig_sem = {};
	vk_semaphore::data acq_wait_sem = {};
	uint64_t acq_wait_sem_val = 0;
	float clear_color[4] = { 0,0,0,0 };
	host_buffer_memory::data staging_buffer = {};
	vk_image::data staging_image = {};
	VkDeviceMemory stating_image_memory = VK_NULL_HANDLE;
};

struct AppState {
	SDL_Window* window = nullptr;
	VulkanState vk_state = {};
	bool is_shutting_down = false;
};

#define SDL_CHECK(result)						\
	if (!result) {									\
		SDL_Log("%s\n", SDL_GetError());		\
		return SDL_APP_FAILURE;					\
	}

static std::unique_ptr<AppState> app_state = std::make_unique<AppState>();

static DWORD render(LPVOID Parameter)
{
	return 0;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
	SDL_CHECK(SDL_Init(SDL_INIT_VIDEO));
	app_state->window = SDL_CreateWindow("Chizen", 100, 100, SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

	int width = 0, height = 0;
	SDL_CHECK(SDL_GetWindowSize(app_state->window, &width, &height));

	if (app_state->window == nullptr)
	{
		SDL_Log("%s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	Uint32 exts_count = 0;
	const char* const* exts = SDL_Vulkan_GetInstanceExtensions(&exts_count);
	app_state->vk_state.instance = vk_instance::create(exts, exts_count);
	SDL_CHECK(SDL_Vulkan_CreateSurface(app_state->window, app_state->vk_state.instance, nullptr, &app_state->vk_state.surface_data.surface));

	app_state->vk_state.phy_dev_data = vk_phydev::get_phy_dev(app_state->vk_state.instance, &app_state->vk_state.surface_data);
	app_state->vk_state.device_data = vk_device::create(app_state->vk_state.phy_dev_data, app_state->vk_state.phy_dev_data.q_fly_idx, app_state->vk_state.phy_dev_data.q_count);
	app_state->vk_state.swapchain_data = vk_swapchain::create(app_state->vk_state.device_data.device, app_state->vk_state.surface_data, app_state->vk_state.phy_dev_data, VK_NULL_HANDLE, "swapchain");
	app_state->vk_state.xfer_cmd_pool_data = vk_command_pool::create(app_state->vk_state.device_data.device, app_state->vk_state.phy_dev_data.q_fly_idx, 1, "xfer command pool");
	app_state->vk_state.acq_sig_sem = vk_semaphore::create(app_state->vk_state.device_data.device, VK_SEMAPHORE_TYPE_BINARY, "acq sig sem");
	app_state->vk_state.acq_wait_sem = vk_semaphore::create(app_state->vk_state.device_data.device, VK_SEMAPHORE_TYPE_TIMELINE, "acq wait sem");
	app_state->vk_state.staging_buffer = host_buffer_memory::create(app_state->vk_state.device_data.device, app_state->vk_state.phy_dev_data.mem_props, 1280 * 720 * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, "staging image");
	app_state->vk_state.staging_image = vk_image::create(app_state->vk_state.device_data.device, { .width = 1280, .height = 720, .depth = 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, "staging image", true, app_state->vk_state.phy_dev_data.mem_props);

	// signalling so that the render function does not stall on vkAcquireNextImage the first time
	const VkSemaphoreSignalInfo sem_sig_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
		.semaphore = app_state->vk_state.acq_wait_sem.semaphore,
		.value = ++app_state->vk_state.acq_wait_sem_val,
	};
	VK_CHECK("signal acq wait sem", vkSignalSemaphore(app_state->vk_state.device_data.device, &sem_sig_info));

	*appstate = app_state.get();

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	AppState* app_state = reinterpret_cast<AppState*>(appstate);

	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_MOUSE_MOTION)
	{
		app_state->vk_state.clear_color[0] = event->motion.x / app_state->vk_state.surface_data.surf_caps.currentExtent.width;
		app_state->vk_state.clear_color[1] = event->motion.y / app_state->vk_state.surface_data.surf_caps.currentExtent.height;
		app_state->vk_state.clear_color[2] = 0;
		app_state->vk_state.clear_color[3] = 1;
	}
	else if (event->type == SDL_EVENT_WINDOW_RESIZED)
	{
		VK_CHECK("wait for present fence", vkWaitForFences(app_state->vk_state.device_data.device, 1, &app_state->vk_state.swapchain_data.present_fences[app_state->vk_state.swapchain_data.curr_img_idx], VK_TRUE, UINT64_MAX));
		VK_CHECK("reset present fence", vkResetFences(app_state->vk_state.device_data.device, 1, &app_state->vk_state.swapchain_data.present_fences[app_state->vk_state.swapchain_data.curr_img_idx]));

		VK_CHECK("get surface capabilities", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app_state->vk_state.phy_dev_data.phy_dev, app_state->vk_state.surface_data.surface, &app_state->vk_state.surface_data.surf_caps));

		vk_swapchain::destroy(app_state->vk_state.swapchain_data, app_state->vk_state.device_data.device);
		app_state->vk_state.swapchain_data = vk_swapchain::create(app_state->vk_state.device_data.device, app_state->vk_state.surface_data, app_state->vk_state.phy_dev_data, VK_NULL_HANDLE, "swapchain");
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	AppState* app_state = reinterpret_cast<AppState*>(appstate);

	// Begin Frame
	VkSemaphoreWaitInfo wait_info = {};
	wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	wait_info.semaphoreCount = 1;
	wait_info.pSemaphores = &app_state->vk_state.acq_wait_sem.semaphore;
	wait_info.pValues = &app_state->vk_state.acq_wait_sem_val;
	vkWaitSemaphores(app_state->vk_state.device_data.device, &wait_info, UINT64_MAX);

	VK_CHECK("acquire image index", vkAcquireNextImageKHR(app_state->vk_state.device_data.device, app_state->vk_state.swapchain_data.swapchain, UINT64_MAX, app_state->vk_state.acq_sig_sem.semaphore, VK_NULL_HANDLE, &app_state->vk_state.swapchain_data.curr_img_idx));
	VK_CHECK("reset command buffer", vkResetCommandBuffer(app_state->vk_state.swapchain_data.cmd_buffs[app_state->vk_state.swapchain_data.curr_img_idx], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

	VkImageMemoryBarrier2 sc_img_mem_bar = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = 0,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = app_state->vk_state.swapchain_data.images[app_state->vk_state.swapchain_data.curr_img_idx],
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		}
	};

	VkDependencyInfo dep_info = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &sc_img_mem_bar,
	};

	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	vkBeginCommandBuffer(app_state->vk_state.swapchain_data.cmd_buffs[app_state->vk_state.swapchain_data.curr_img_idx], &begin_info);
	vkCmdPipelineBarrier2(app_state->vk_state.swapchain_data.cmd_buffs[app_state->vk_state.swapchain_data.curr_img_idx], &dep_info);

	// Clear frame
	const VkRenderingAttachmentInfoKHR color_attachment_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = app_state->vk_state.swapchain_data.image_views[app_state->vk_state.swapchain_data.curr_img_idx],
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = {
				.color = {
					.float32 = {
						app_state->vk_state.clear_color[0],
						app_state->vk_state.clear_color[1],
						app_state->vk_state.clear_color[2],
						app_state->vk_state.clear_color[3],
					},
				},
			},
		},
	};

	const VkRenderingInfoKHR rendering_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = {
			.extent = app_state->vk_state.surface_data.surf_caps.currentExtent,
		},
		.layerCount = 1,
		.colorAttachmentCount = _countof(color_attachment_infos),
		.pColorAttachments = color_attachment_infos,
	};

	// Render frame
	vkCmdBeginRendering(app_state->vk_state.swapchain_data.cmd_buffs[app_state->vk_state.swapchain_data.curr_img_idx], &rendering_info);

	// End Frame
	vkCmdEndRendering(app_state->vk_state.swapchain_data.cmd_buffs[app_state->vk_state.swapchain_data.curr_img_idx]);

	// Update staging image
	uint8_t* pixels = reinterpret_cast<uint8_t*>(app_state->vk_state.staging_buffer.map);

	for (size_t y = 0; y < 720; ++y)
	{
		for (size_t x = 0; x < 1280; ++x)
		{
			size_t pixel_idx = (y * 1280 * 4) + (x * 4);

			pixels[pixel_idx] = static_cast<uint8_t>(static_cast<float>(rand()) / RAND_MAX * 255);
			pixels[pixel_idx + 1] = static_cast<uint8_t>(static_cast<float>(rand()) / RAND_MAX * 255);
			pixels[pixel_idx + 2] = static_cast<uint8_t>(static_cast<float>(rand()) / RAND_MAX * 255);
			pixels[pixel_idx + 3] = 255;
		}
	}

	// Copy staging buffer to staging image
	// change staging image layout to xfer dst
	const VkImageMemoryBarrier2 stg_img_to_xfer_dst_bar = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.image = app_state->vk_state.staging_image.image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		}
	};

	const VkDependencyInfo stg_img_to_xfer_dst_dep = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &stg_img_to_xfer_dst_bar,
	};
	vkCmdPipelineBarrier2(app_state->vk_state.swapchain_data.cmd_buffs[app_state->vk_state.swapchain_data.curr_img_idx], &stg_img_to_xfer_dst_dep);

	// copy stg buffer to stg img
	const VkBufferImageCopy2 stg_buff_stg_img_regions[] = {
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.layerCount = 1,
			},
			.imageExtent = {
				.width = 1280,
				.height = 720,
				.depth = 1,
			}
		},
	};

	const VkCopyBufferToImageInfo2 copy_stg_buff_to_stg_img_info = {
		.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
		.srcBuffer = app_state->vk_state.staging_buffer.buffer,
		.dstImage = app_state->vk_state.staging_image.image,
		.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.regionCount = _countof(stg_buff_stg_img_regions),
		.pRegions = stg_buff_stg_img_regions,
	};
	vkCmdCopyBufferToImage2(app_state->vk_state.swapchain_data.cmd_buffs[app_state->vk_state.swapchain_data.curr_img_idx], &copy_stg_buff_to_stg_img_info);

	// change stg img layout to xfer src
	const VkImageMemoryBarrier2 stg_img_to_xfer_src_bar = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
		.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		.image = app_state->vk_state.staging_image.image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		}
	};

	const VkDependencyInfo stg_img_to_xfer_src_dep = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &stg_img_to_xfer_src_bar,
	};
	vkCmdPipelineBarrier2(app_state->vk_state.swapchain_data.cmd_buffs[app_state->vk_state.swapchain_data.curr_img_idx], &stg_img_to_xfer_src_dep);

	// Copy stg img to sc img
	// change sc img layout to xfer dst
	const VkImageMemoryBarrier2 sc_img_to_dst_bar = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.image = app_state->vk_state.swapchain_data.images[app_state->vk_state.swapchain_data.curr_img_idx],
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		}
	};

	const VkDependencyInfo sc_img_to_dst_dep = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &sc_img_to_dst_bar,
	};
	vkCmdPipelineBarrier2(app_state->vk_state.swapchain_data.cmd_buffs[app_state->vk_state.swapchain_data.curr_img_idx], &sc_img_to_dst_dep);

	const VkImageBlit2 blit_regions[] = {
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
			.srcSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.layerCount = 1,
			},
			.srcOffsets = {
				{},{.x = 1280, .y = 720, .z = 1}
			},
			.dstSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.layerCount = 1,
			},
			.dstOffsets = {
				{},{.x = 1280, .y = 720, .z = 1}
			},
		},
	};

	// blit stg img to sc img
	const VkBlitImageInfo2 stg_img_to_sc_img_info = {
		.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
		.srcImage = app_state->vk_state.staging_image.image,
		.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		.dstImage = app_state->vk_state.swapchain_data.images[app_state->vk_state.swapchain_data.curr_img_idx],
		.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.regionCount = _countof(blit_regions),
		.pRegions = blit_regions,
	};

	vkCmdBlitImage2(app_state->vk_state.swapchain_data.cmd_buffs[app_state->vk_state.swapchain_data.curr_img_idx], &stg_img_to_sc_img_info);

	sc_img_mem_bar.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	sc_img_mem_bar.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	sc_img_mem_bar.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
	sc_img_mem_bar.dstAccessMask = 0;
	sc_img_mem_bar.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	sc_img_mem_bar.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	sc_img_mem_bar.image = app_state->vk_state.swapchain_data.images[app_state->vk_state.swapchain_data.curr_img_idx];
	sc_img_mem_bar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	sc_img_mem_bar.subresourceRange.levelCount = 1;
	sc_img_mem_bar.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier2(app_state->vk_state.swapchain_data.cmd_buffs[app_state->vk_state.swapchain_data.curr_img_idx], &dep_info);

	VK_CHECK("end buffer", vkEndCommandBuffer(app_state->vk_state.swapchain_data.cmd_buffs[app_state->vk_state.swapchain_data.curr_img_idx]));

	const VkSemaphoreSubmitInfo wait_sem_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = app_state->vk_state.acq_sig_sem.semaphore,
			.stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		},
	};

	const VkSemaphoreSubmitInfo sig_sem_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = app_state->vk_state.swapchain_data.rndr_semaphores[app_state->vk_state.swapchain_data.curr_img_idx],
			.stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		},
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = app_state->vk_state.acq_wait_sem.semaphore,
			.value = ++app_state->vk_state.acq_wait_sem_val,
			.stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		},
	};

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = app_state->vk_state.swapchain_data.cmd_buffs[app_state->vk_state.swapchain_data.curr_img_idx],
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

	VK_CHECK("queue submit", vkQueueSubmit2(app_state->vk_state.device_data.gfx_q, 1, &submit_info, VK_NULL_HANDLE));

	const VkSwapchainPresentFenceInfoEXT present_fence_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT,
		.swapchainCount = 1,
		.pFences = &app_state->vk_state.swapchain_data.present_fences[app_state->vk_state.swapchain_data.curr_img_idx],
	};

	const VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = &present_fence_info,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &app_state->vk_state.swapchain_data.rndr_semaphores[app_state->vk_state.swapchain_data.curr_img_idx],
		.swapchainCount = 1,
		.pSwapchains = &app_state->vk_state.swapchain_data.swapchain,
		.pImageIndices = &app_state->vk_state.swapchain_data.curr_img_idx,
	};

	VK_CHECK("reset present fence", vkResetFences(app_state->vk_state.device_data.device, 1, &app_state->vk_state.swapchain_data.present_fences[app_state->vk_state.swapchain_data.curr_img_idx]));
	VK_CHECK("queue present", vkQueuePresentKHR(app_state->vk_state.device_data.gfx_q, &present_info));

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	AppState* app_state = (AppState*)appstate;

	VK_CHECK("wait for present fence", vkWaitForFences(app_state->vk_state.device_data.device, 1, &app_state->vk_state.swapchain_data.present_fences[app_state->vk_state.swapchain_data.curr_img_idx], VK_TRUE, UINT64_MAX));
	VK_CHECK("reset present fence", vkResetFences(app_state->vk_state.device_data.device, 1, &app_state->vk_state.swapchain_data.present_fences[app_state->vk_state.swapchain_data.curr_img_idx]));

	vk_image::destroy(app_state->vk_state.staging_image, app_state->vk_state.device_data.device);
	host_buffer_memory::destroy(app_state->vk_state.staging_buffer, app_state->vk_state.device_data.device);
	vk_semaphore::destroy(app_state->vk_state.acq_sig_sem.semaphore, app_state->vk_state.device_data.device);
	vk_semaphore::destroy(app_state->vk_state.acq_wait_sem.semaphore, app_state->vk_state.device_data.device);
	vk_command_pool::destroy(app_state->vk_state.xfer_cmd_pool_data, app_state->vk_state.device_data.device);
	vk_swapchain::destroy(app_state->vk_state.swapchain_data, app_state->vk_state.device_data.device);
	vk_device::destroy(app_state->vk_state.device_data.device);
	SDL_Vulkan_DestroySurface(app_state->vk_state.instance, app_state->vk_state.surface_data.surface, nullptr);
	vk_instance::destroy(app_state->vk_state.instance);
}