#include "vk_objects.hpp"

struct VulkanState {
	VkInstance instance = VK_NULL_HANDLE;
	vk_surface::data surface_data = {};
	vk_phydev::data phy_dev_data = {};
	vk_device::data device_data = {};
	vk_swapchain::data swapchain_data = {};
	vk_command_pool::data xfer_cmd_pool_data = {};
	float clear_color[4] = { 0,0,0,0 };
	host_buffer_memory::data staging_buffer = {};
	vk_image::data staging_image = {};
	VkDeviceMemory stating_image_memory = VK_NULL_HANDLE;
	VkDescriptorPool imgui_pool = VK_NULL_HANDLE;
};

struct AppState {
	SDL_Window* window = nullptr;
	VulkanState vk_state = {};
	bool is_shutting_down = false;
	std::thread render_thread = {};
};

#define SDL_CHECK(result)						\
	if (!result) {									\
		SDL_Log("%s\n", SDL_GetError());		\
		return SDL_APP_FAILURE;					\
	}

static AppState app_state = {};

void render()
{
	std::println("rendering");
	for (size_t i = 0; i < 100; ++i)
	{
		if (app_state.is_shutting_down)
			break;

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		std::println("{}", i);
	}
	return;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
	SDL_CHECK(SDL_Init(SDL_INIT_VIDEO));
	app_state.window = SDL_CreateWindow("Chizen", 1280, 720, SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

	int width = 0, height = 0;
	SDL_CHECK(SDL_GetWindowSize(app_state.window, &width, &height));

	if (app_state.window == nullptr)
	{
		SDL_Log("%s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	Uint32 exts_count = 0;
	const char* const* exts = SDL_Vulkan_GetInstanceExtensions(&exts_count);
	app_state.vk_state.instance = vk_instance::create(exts, exts_count);
	SDL_CHECK(SDL_Vulkan_CreateSurface(app_state.window, app_state.vk_state.instance, nullptr, &app_state.vk_state.surface_data.surface));

	app_state.vk_state.phy_dev_data = vk_phydev::get_phy_dev(app_state.vk_state.instance, &app_state.vk_state.surface_data);
	app_state.vk_state.device_data = vk_device::create(app_state.vk_state.phy_dev_data, app_state.vk_state.phy_dev_data.q_fly_idx, app_state.vk_state.phy_dev_data.q_count);
	app_state.vk_state.swapchain_data = vk_swapchain::create(app_state.vk_state.device_data.device, app_state.vk_state.surface_data, app_state.vk_state.phy_dev_data, VK_NULL_HANDLE, "swapchain");
	app_state.vk_state.xfer_cmd_pool_data = vk_command_pool::create(app_state.vk_state.device_data.device, app_state.vk_state.phy_dev_data.q_fly_idx, 1, "xfer command pool");
	app_state.vk_state.staging_buffer = host_buffer_memory::create(app_state.vk_state.device_data.device, app_state.vk_state.phy_dev_data.mem_props, 1280 * 720 * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, "staging image");
	app_state.vk_state.staging_image = vk_image::create(app_state.vk_state.device_data.device, { .width = 1280, .height = 720, .depth = 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, "staging image", true, app_state.vk_state.phy_dev_data.mem_props);

	const VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	const	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 1000,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes,
	};

	VK_CHECK("create imgui desc pool", vkCreateDescriptorPool(app_state.vk_state.device_data.device, &pool_info, nullptr, &app_state.vk_state.imgui_pool));

	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	SDL_CHECK(ImGui_ImplSDL3_InitForVulkan(app_state.window));

	ImGui_ImplVulkan_InitInfo imgui_init_info = {
		.Instance = app_state.vk_state.instance,
		.PhysicalDevice = app_state.vk_state.phy_dev_data.phy_dev,
		.Device = app_state.vk_state.device_data.device,
		.Queue = app_state.vk_state.device_data.gfx_q,
		.DescriptorPool = app_state.vk_state.imgui_pool,
		.MinImageCount = app_state.vk_state.swapchain_data.images_count,
		.ImageCount = app_state.vk_state.swapchain_data.images_count,
		.PipelineInfoMain = {
			.PipelineRenderingCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
				.colorAttachmentCount = 1,
				.pColorAttachmentFormats = &app_state.vk_state.surface_data.format.format,
			},
		},
		.UseDynamicRendering = true,
	};

	SDL_CHECK(ImGui_ImplVulkan_Init(&imgui_init_info));


	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	ImGui_ImplSDL3_ProcessEvent(event);

	if (event->type == SDL_EVENT_QUIT)
	{
		app_state.is_shutting_down = true;
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_MOUSE_MOTION)
	{
		app_state.vk_state.clear_color[0] = event->motion.x / app_state.vk_state.surface_data.surf_caps.currentExtent.width;
		app_state.vk_state.clear_color[1] = event->motion.y / app_state.vk_state.surface_data.surf_caps.currentExtent.height;
		app_state.vk_state.clear_color[2] = 0;
		app_state.vk_state.clear_color[3] = 1;
	}
	else if (event->type == SDL_EVENT_KEY_DOWN)
	{
		if (event->key.key == SDLK_R)
		{
			app_state.render_thread = std::thread(render);
			app_state.render_thread.detach();
		}
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	VkDevice device = app_state.vk_state.device_data.device;

	const VkAcquireNextImageInfoKHR acq_info = {
		.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
		.swapchain = app_state.vk_state.swapchain_data.swapchain,
		.semaphore = app_state.vk_state.swapchain_data.acq_sig_sem.semaphore,
		.deviceMask = 0x1,
	};

	VkResult result = vkAcquireNextImage2KHR(device, &acq_info, &app_state.vk_state.swapchain_data.curr_img_idx);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		VK_CHECK("gfx q wait idle", vkQueueWaitIdle(app_state.vk_state.device_data.gfx_q));
		VK_CHECK("get surface capabilities", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app_state.vk_state.phy_dev_data.phy_dev, app_state.vk_state.surface_data.surface, &app_state.vk_state.surface_data.surf_caps));

		if (SDL_GetWindowFlags(app_state.window) & SDL_WINDOW_MINIMIZED)
		{
			return SDL_APP_CONTINUE;
		}

		vk_swapchain::destroy(app_state.vk_state.swapchain_data, app_state.vk_state.device_data.device);
		app_state.vk_state.swapchain_data = vk_swapchain::create(app_state.vk_state.device_data.device, app_state.vk_state.surface_data, app_state.vk_state.phy_dev_data, VK_NULL_HANDLE, "swapchain");

		return SDL_APP_CONTINUE;
	}

	VkCommandBuffer curr_cmd_buff = app_state.vk_state.swapchain_data.cmd_buffs[app_state.vk_state.swapchain_data.curr_img_idx];
	VkImage curr_sc_img = app_state.vk_state.swapchain_data.images[app_state.vk_state.swapchain_data.curr_img_idx];
	VkFence curr_rndr_fnc = app_state.vk_state.swapchain_data.rndr_fncs[app_state.vk_state.swapchain_data.curr_img_idx];

	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VK_CHECK("begin sc cmd_buff", vkBeginCommandBuffer(curr_cmd_buff, &begin_info));

	change_image_layout(curr_cmd_buff,
		0, 0,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		curr_sc_img);

	const VkRenderingAttachmentInfo col_attachs[] = {
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = app_state.vk_state.swapchain_data.image_views[app_state.vk_state.swapchain_data.curr_img_idx],
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = {
				.color = {
					.float32 = {
						app_state.vk_state.clear_color[0],
						app_state.vk_state.clear_color[1],
						app_state.vk_state.clear_color[2],
						app_state.vk_state.clear_color[3]
					},
				},
			},
		},
	};

	const VkRenderingInfo rendering_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = {
			.extent = app_state.vk_state.surface_data.surf_caps.currentExtent,
		},
		.layerCount = 1,
		.colorAttachmentCount = _countof(col_attachs),
		.pColorAttachments = col_attachs,
	};

	vkCmdBeginRendering(curr_cmd_buff, &rendering_info);
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	ImGui::Render();

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), curr_cmd_buff);

	// Update and Render additional Platform Windows
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
	vkCmdEndRendering(curr_cmd_buff);

	change_image_layout(curr_cmd_buff,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		curr_sc_img);

	VK_CHECK("end sc cmd_buff", vkEndCommandBuffer(curr_cmd_buff));

	const VkSemaphoreSubmitInfo wait_sem_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = app_state.vk_state.swapchain_data.acq_sig_sem.semaphore,
			.stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		},
	};

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = curr_cmd_buff,
		},
	};

	const VkSubmitInfo2 submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = _countof(wait_sem_infos),
			.pWaitSemaphoreInfos = wait_sem_infos,
			.commandBufferInfoCount = _countof(cmd_buff_infos),
			.pCommandBufferInfos = cmd_buff_infos,
		},
	};

	VK_CHECK("submit drawing commamds", vkQueueSubmit2(app_state.vk_state.device_data.gfx_q, _countof(submit_infos), submit_infos, curr_rndr_fnc));

	VK_CHECK("wait for rndr fnc", vkWaitForFences(device, 1, &curr_rndr_fnc, VK_TRUE, UINT64_MAX));
	VK_CHECK("reset rndr fnc", vkResetFences(device, 1, &curr_rndr_fnc));

	const VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.swapchainCount = 1,
		.pSwapchains = &app_state.vk_state.swapchain_data.swapchain,
		.pImageIndices = &app_state.vk_state.swapchain_data.curr_img_idx,
	};

	result = vkQueuePresentKHR(app_state.vk_state.device_data.gfx_q, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		VK_CHECK("gfx q wait idle", vkQueueWaitIdle(app_state.vk_state.device_data.gfx_q));
		VK_CHECK("get surface capabilities", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app_state.vk_state.phy_dev_data.phy_dev, app_state.vk_state.surface_data.surface, &app_state.vk_state.surface_data.surf_caps));

		if (SDL_GetWindowFlags(app_state.window) & SDL_WINDOW_MINIMIZED)
		{
			return SDL_APP_CONTINUE;
		}

		vk_swapchain::destroy(app_state.vk_state.swapchain_data, app_state.vk_state.device_data.device);
		app_state.vk_state.swapchain_data = vk_swapchain::create(app_state.vk_state.device_data.device, app_state.vk_state.surface_data, app_state.vk_state.phy_dev_data, VK_NULL_HANDLE, "swapchain");

		return SDL_APP_CONTINUE;
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	VK_CHECK("device wait idle", vkDeviceWaitIdle(app_state.vk_state.device_data.device));

	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplVulkan_Shutdown();
	ImGui::DestroyContext();

	vkDestroyDescriptorPool(app_state.vk_state.device_data.device, app_state.vk_state.imgui_pool, nullptr);
	vk_image::destroy(app_state.vk_state.staging_image, app_state.vk_state.device_data.device);
	host_buffer_memory::destroy(app_state.vk_state.staging_buffer, app_state.vk_state.device_data.device);
	vk_command_pool::destroy(app_state.vk_state.xfer_cmd_pool_data, app_state.vk_state.device_data.device);
	vk_swapchain::destroy(app_state.vk_state.swapchain_data, app_state.vk_state.device_data.device);
	vk_device::destroy(app_state.vk_state.device_data.device);
	SDL_Vulkan_DestroySurface(app_state.vk_state.instance, app_state.vk_state.surface_data.surface, nullptr);
	vk_instance::destroy(app_state.vk_state.instance);
}