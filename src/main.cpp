#include "vk_objects.hpp"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

struct VulkanState
{
	VkInstance instance = VK_NULL_HANDLE;
	VmaAllocator allocator = VK_NULL_HANDLE;
	vk_surface::data surface_data = {};
	vk_phydev::data phy_dev_data = {};
	vk_device::data device_data = {};
	vk_swapchain::data swapchain_data = {};
	cmpt_swapchain::data cmpt_swapchain_data = {};
	vk_command_pool::data xfer_cmd_pool_data = {};
	float clear_color[4] = { 0,0,0,0 };
	VkDescriptorPool imgui_pool = VK_NULL_HANDLE;
	vk_graphics_pipeline::data gfx_ppln = {};
	vk_buffer::data geom_buffer = {};
	uint32_t max_samples = 1024;
	uint32_t curr_sample = 1;
};

struct ImGuiState
{
	int tmp_render_dims[2] = { 1280, 720 };
	int tmp_max_samples = 1024;
	bool should_be_rendering = false;
};

struct RenderTargetState
{
	VkExtent2D dims = { 1280, 720 };
	float zoom_level = 1;
};

SDL_Window* window = nullptr;
std::string current_path;
VulkanState vk_state = {};
float delta_mouse[2];
RenderTargetState rt_state = {};
float last_mouse_pos[2];
ImGuiState imgui_state = {};
std::thread render_thread;
bool mouse_motion_tracking = false;
bool is_rendering = false;
bool should_stop_rendering = false;
bool is_app_shutting_down = false;

#define SDL_CHECK(result)						\
	if (!result) {									\
		SDL_Log("%s\n", SDL_GetError());		\
		return SDL_APP_FAILURE;					\
	}


SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
	current_path = std::filesystem::path(std::string(argv[0])).parent_path().string();

	SDL_CHECK(SDL_Init(SDL_INIT_VIDEO));
	window = SDL_CreateWindow("Chizen", rt_state.dims.width, rt_state.dims.height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

	if (window == nullptr)
	{
		SDL_Log("%s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	Uint32 exts_count = 0;
	const char* const* exts = SDL_Vulkan_GetInstanceExtensions(&exts_count);
	vk_state.instance = vk_instance::create(exts, exts_count);

	SDL_CHECK(SDL_Vulkan_CreateSurface(window, vk_state.instance, nullptr, &vk_state.surface_data.surface));

	vk_state.phy_dev_data = vk_phydev::get_phy_dev(vk_state.instance, &vk_state.surface_data);
	vk_state.device_data = vk_device::create(vk_state.phy_dev_data);
	vk_state.xfer_cmd_pool_data = vk_command_pool::create(vk_state.device_data.device, vk_state.phy_dev_data.xfer_q_fly_idx, 1, "xfer command pool");

	const VmaAllocatorCreateInfo vma_alloc_ci = {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = vk_state.phy_dev_data.phy_dev,
		.device = vk_state.device_data.device,
		.instance = vk_state.instance,
	};

	VK_CHECK("create vma allocator", vmaCreateAllocator(&vma_alloc_ci, &vk_state.allocator));

	vk_state.swapchain_data = vk_swapchain::create(vk_state.device_data.device, vk_state.surface_data, vk_state.phy_dev_data.gfx_q_fly_idx, VK_NULL_HANDLE, "swapchain");
	vk_state.cmpt_swapchain_data = cmpt_swapchain::create(vk_state.device_data.device, 
		{ rt_state.dims.width, rt_state.dims.height, 1 }, 
		vk_state.allocator, current_path, vk_state.phy_dev_data.cmpt_q_fly_idx, 
		{ vk_state.phy_dev_data.cmpt_q_fly_idx, vk_state.phy_dev_data.gfx_q_fly_idx , vk_state.phy_dev_data.xfer_q_fly_idx }, vk_state.xfer_cmd_pool_data.cmd_buffs[0], vk_state.device_data.xfer_q, "cmpt swapchain");

	vk_state.gfx_ppln = vk_graphics_pipeline::create(vk_state.device_data.device, current_path, vk_state.surface_data.format.format, vk_state.swapchain_data.max_frames_in_flight, "graphics pipeline");

	float verts[] = {
		// Positions (X, Y) | UVs (U, V)
		-1.0f,  1.0f,  0.0f, 1.0f, // top-left
		-1.0f, -1.0f,  0.0f, 0.0f, // bottom-left
		 1.0f, -1.0f,  1.0f, 0.0f, // bottom-right

		-1.0f,  1.0f,  0.0f, 1.0f, // top-left
		 1.0f, -1.0f,  1.0f, 0.0f, // bottom-right
		 1.0f,  1.0f,  1.0f, 1.0f  // top-right
	};

	size_t verts_size = std::size(verts) * sizeof(float);

	vk_state.geom_buffer = vk_buffer::create(vk_state.device_data.device, vk_state.allocator, verts_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, "geom buffer");
	vk_buffer::data geom_staging_buffer = vk_buffer::create(vk_state.device_data.device, vk_state.allocator, verts_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, "staging geom buffer");

	memcpy(geom_staging_buffer.alloc_info.pMappedData, verts, verts_size);

	copy_buffer_to_buffer(vk_state.xfer_cmd_pool_data.cmd_buffs[0], vk_state.device_data.xfer_q, geom_staging_buffer.buffer, vk_state.geom_buffer.buffer, verts_size);

	vk_buffer::destroy(geom_staging_buffer, vk_state.allocator, vk_state.device_data.device);

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

	VK_CHECK("create imgui desc pool", vkCreateDescriptorPool(vk_state.device_data.device, &pool_info, nullptr, &vk_state.imgui_pool));

	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	SDL_CHECK(ImGui_ImplSDL3_InitForVulkan(window));

	ImGui_ImplVulkan_InitInfo imgui_init_info = {
		.Instance = vk_state.instance,
		.PhysicalDevice = vk_state.phy_dev_data.phy_dev,
		.Device = vk_state.device_data.device,
		.Queue = vk_state.device_data.gfx_q,
		.DescriptorPool = vk_state.imgui_pool,
		.MinImageCount = vk_state.surface_data.surf_caps.minImageCount,
		.ImageCount = vk_state.surface_data.surf_caps.minImageCount,
		.PipelineInfoMain = {
			.PipelineRenderingCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
				.colorAttachmentCount = 1,
				.pColorAttachmentFormats = &vk_state.surface_data.format.format,
			},
		},
		.UseDynamicRendering = true,
	};

	SDL_CHECK(ImGui_ImplVulkan_Init(&imgui_init_info));

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
	{
		return SDL_APP_CONTINUE;
	}

	ImGui_ImplSDL3_ProcessEvent(event);

	ImGuiIO& io = ImGui::GetIO();

	if (event->type == SDL_EVENT_QUIT)
	{
		is_app_shutting_down = true;
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
	{
		if (!io.WantCaptureMouse)
		{
			mouse_motion_tracking = true;
			last_mouse_pos[0] = event->motion.x;
			last_mouse_pos[1] = event->motion.y;
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
	{
		if (!io.WantCaptureMouse)
		{
			mouse_motion_tracking = false;
			last_mouse_pos[0] = 0;
			last_mouse_pos[1] = 0;
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_MOTION)
	{
		if (!io.WantCaptureMouse)
		{
			if (mouse_motion_tracking)
			{
				delta_mouse[0] += ((last_mouse_pos[0] - event->motion.x) / static_cast<float>(vk_state.surface_data.surf_caps.currentExtent.width)) * 2,
				delta_mouse[1] += ((last_mouse_pos[1] - event->motion.y) / static_cast<float>(vk_state.surface_data.surf_caps.currentExtent.height)) * 2,

				last_mouse_pos[0] = event->motion.x;
				last_mouse_pos[1] = event->motion.y;

				imgui_state.should_be_rendering = true;
			}
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_WHEEL)
	{
		if (!io.WantCaptureMouse)
		{
			rt_state.zoom_level = std::max(0.01f, rt_state.zoom_level + event->wheel.y / 20.f);
		}
	}
	else if (event->type == SDL_EVENT_WINDOW_RESIZED)
	{
		VK_CHECK("gfx q wait idle", vkQueueWaitIdle(vk_state.device_data.gfx_q));
		VK_CHECK("get surface capabilities", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_state.phy_dev_data.phy_dev, vk_state.surface_data.surface, &vk_state.surface_data.surf_caps));
		vk_state.swapchain_data = vk_swapchain::recreate_swapchain(vk_state.swapchain_data, vk_state.device_data.device, vk_state.surface_data, vk_state.phy_dev_data.gfx_q_fly_idx, "swapchain");
	}

	return SDL_APP_CONTINUE;
}

void render()
{
	is_rendering = true;

	VkDevice device = vk_state.device_data.device;
	uint32_t s = vk_state.curr_sample;

	do
	{
		if (is_app_shutting_down || should_stop_rendering)
		{
			VK_CHECK("cmpt queue idle", vkQueueWaitIdle(vk_state.device_data.cmpt_q));
			break;
		}

		uint8_t frame_in_flight = vk_state.cmpt_swapchain_data.frame_in_flight;

		const VkSemaphoreWaitInfo cmpt_wait_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &vk_state.cmpt_swapchain_data.frame_sems[frame_in_flight],
			.pValues = &vk_state.cmpt_swapchain_data.frame_sem_vals[frame_in_flight],
		};

		VK_CHECK("wait before cmpt cmd buff", vkWaitSemaphores(device, &cmpt_wait_info, UINT64_MAX));

		VkCommandBuffer cmpt_cmd_buff = vk_state.cmpt_swapchain_data.cmd_buffs[frame_in_flight];

		const VkCommandBufferBeginInfo cmpt_begin_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};
		VK_CHECK("begin cmpt cmd_buff", vkBeginCommandBuffer(cmpt_cmd_buff, &cmpt_begin_info));

		insert_memory_barrier(cmpt_cmd_buff,
			VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT
		);
		insert_memory_barrier(cmpt_cmd_buff,
			VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT
		);

		if (s == 1)
		{
			const VkClearColorValue clear_color = {
				.float32 = {
					0, 0, 0, 1,
				},
			};

			const VkImageSubresourceRange ranges[] = {
				{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1,
				},
			};

			vkCmdClearColorImage(cmpt_cmd_buff, vk_state.cmpt_swapchain_data.accum_target.image, VK_IMAGE_LAYOUT_GENERAL, &clear_color, std::size(ranges), ranges);
			vkCmdClearColorImage(cmpt_cmd_buff, vk_state.cmpt_swapchain_data.final_render.image, VK_IMAGE_LAYOUT_GENERAL, &clear_color, std::size(ranges), ranges);
		}

		vkCmdBindPipeline(cmpt_cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, vk_state.cmpt_swapchain_data.ppln_data.pipe);

		const VkWriteDescriptorSet cmpt_desc_writes[] = {
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = vk_state.cmpt_swapchain_data.ppln_data.dss[frame_in_flight],
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.pImageInfo = &vk_state.cmpt_swapchain_data.accum_target.desc_img_info,
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = vk_state.cmpt_swapchain_data.ppln_data.dss[frame_in_flight],
				.dstBinding = 1,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.pImageInfo = &vk_state.cmpt_swapchain_data.final_render.desc_img_info,
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = vk_state.cmpt_swapchain_data.ppln_data.dss[frame_in_flight],
				.dstBinding = 2,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &vk_state.cmpt_swapchain_data.rand_states.desc_info,
			},
		};

		vkUpdateDescriptorSets(device, std::size(cmpt_desc_writes), cmpt_desc_writes, 0, nullptr);

		const VkBindDescriptorSetsInfo cmpt_ds_bi = {
			.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
			.layout = vk_state.cmpt_swapchain_data.ppln_data.lyt,
			.descriptorSetCount = 1,
			.pDescriptorSets = &vk_state.cmpt_swapchain_data.ppln_data.dss[frame_in_flight],
		};

		vkCmdBindDescriptorSets2(cmpt_cmd_buff, &cmpt_ds_bi);

		const vk_compute_pipeline::PushConstants cmpt_pc = {
			.dispatch_x = rt_state.dims.width,
			.dispatch_y = rt_state.dims.height,
			.curr_sample = s,
		};

		const VkPushConstantsInfo cmpt_pc_info = {
			.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
			.layout = vk_state.cmpt_swapchain_data.ppln_data.lyt,
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
			.size = sizeof(vk_compute_pipeline::PushConstants),
			.pValues = &cmpt_pc,
		};

		vkCmdPushConstants2(cmpt_cmd_buff, &cmpt_pc_info);

		vkCmdDispatch(cmpt_cmd_buff, cmpt_pc.dispatch_x / 32 + 1, cmpt_pc.dispatch_y / 32 + 1, 1);

		VK_CHECK("end cmpt cmd buffer", vkEndCommandBuffer(cmpt_cmd_buff));

		const VkSemaphoreSubmitInfo cmpt_wait_sem_infos[] = {
			{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = vk_state.cmpt_swapchain_data.frame_sems[frame_in_flight],
				.value = vk_state.cmpt_swapchain_data.frame_sem_vals[frame_in_flight],
				.stageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			}
		};

		const VkCommandBufferSubmitInfo cmpt_cmd_buff_infos[] = {
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
				.commandBuffer = cmpt_cmd_buff,
			},
		};

		const VkSemaphoreSubmitInfo cmpt_sig_sem_infos[] = {
			{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = vk_state.cmpt_swapchain_data.frame_sems[frame_in_flight],
				.value = ++vk_state.cmpt_swapchain_data.frame_sem_vals[frame_in_flight],
				.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			},
		};

		const VkSubmitInfo2 cmpt_submit_infos[] = {
			{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
				.waitSemaphoreInfoCount = std::size(cmpt_wait_sem_infos),
				.pWaitSemaphoreInfos = cmpt_wait_sem_infos,
				.commandBufferInfoCount = std::size(cmpt_cmd_buff_infos),
				.pCommandBufferInfos = cmpt_cmd_buff_infos,
				.signalSemaphoreInfoCount = std::size(cmpt_sig_sem_infos),
				.pSignalSemaphoreInfos = cmpt_sig_sem_infos,
			},
		};

		VK_CHECK("submit compute commamds", vkQueueSubmit2(vk_state.device_data.cmpt_q, std::size(cmpt_submit_infos), cmpt_submit_infos, VK_NULL_HANDLE));

		vk_state.cmpt_swapchain_data.frame_in_flight = (frame_in_flight + 1) % vk_state.cmpt_swapchain_data.max_frames_in_flight;
	} while (++s <= vk_state.max_samples);

	is_rendering = false;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
	{
		return SDL_APP_CONTINUE;
	}

	VkDevice device = vk_state.device_data.device;
	uint8_t frame_in_flight = 0;// vk_state.swapchain_data.gfx_frame_in_flight; // setting this to 0 and using vkQueuWaitIdle lets the compute fly !?!?!!? WIERD!!!!

	const VkSemaphoreWaitInfo wait_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		.semaphoreCount = 1,
		.pSemaphores = &vk_state.swapchain_data.gfx_frame_sems[frame_in_flight],
		.pValues = &vk_state.swapchain_data.gfx_frame_sem_vals[frame_in_flight],
	};

	VK_CHECK("wait acq img", vkWaitSemaphores(device, &wait_info, UINT64_MAX));

	const VkAcquireNextImageInfoKHR acq_info = {
		.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
		.swapchain = vk_state.swapchain_data.swapchain,
		.timeout = UINT64_MAX,
		.semaphore = vk_state.swapchain_data.acq_sig_sems[frame_in_flight],
		.deviceMask = 0x1,
	};

	uint32_t sc_img_idx = 0;
	VK_CHECK("acq img idx", vkAcquireNextImage2KHR(device, &acq_info, &sc_img_idx));

	VkCommandBuffer gfx_cmd_buff = vk_state.swapchain_data.gfx_cmd_buffs[frame_in_flight];
	VkImage curr_sc_img = vk_state.swapchain_data.images[sc_img_idx];

	const VkCommandBufferBeginInfo gfx_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VK_CHECK("begin gfx cmd_buff", vkBeginCommandBuffer(gfx_cmd_buff, &gfx_begin_info));

	if (is_rendering)
	{
		insert_memory_barrier(gfx_cmd_buff,
			VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT
		);
	}

	change_image_layout(gfx_cmd_buff,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		curr_sc_img
	);

	VkRenderingAttachmentInfo col_attachs[] = {
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = vk_state.swapchain_data.image_views[sc_img_idx],
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = {
				.color = {
					.float32 = {
						0.2f,
						0.2f,
						0.2f,
						1.0f,
					},
				},
			},
		},
	};

	const VkRenderingInfo rendering_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = {
			.extent = vk_state.surface_data.surf_caps.currentExtent,
		},
		.layerCount = 1,
		.colorAttachmentCount = std::size(col_attachs),
		.pColorAttachments = col_attachs,
	};

	vkCmdBeginRendering(gfx_cmd_buff, &rendering_info);

	vkCmdBindPipeline(gfx_cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_state.gfx_ppln.pipe);

	const VkBindDescriptorSetsInfo gfx_ds_bi = {
		.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.layout = vk_state.gfx_ppln.lyt,
		.descriptorSetCount = 1,
		.pDescriptorSets = &vk_state.gfx_ppln.dss[frame_in_flight],
	};

	const VkWriteDescriptorSet gfx_desc_writes[] = {
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = vk_state.gfx_ppln.dss[frame_in_flight],
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &vk_state.cmpt_swapchain_data.final_render.desc_img_info,
		},
	};

	vkUpdateDescriptorSets(device, std::size(gfx_desc_writes), gfx_desc_writes, 0, nullptr);
	vkCmdBindDescriptorSets2(gfx_cmd_buff, &gfx_ds_bi);

	const VkViewport viewports[] = {
		{
			.width = static_cast<float>(vk_state.surface_data.surf_caps.currentExtent.width),
			.height = static_cast<float>(vk_state.surface_data.surf_caps.currentExtent.height),
			.maxDepth = 1.f,
		},
	};

	const VkRect2D scissors[] = {
		{
			.extent = vk_state.surface_data.surf_caps.currentExtent,
		},
	};

	vkCmdSetScissor(gfx_cmd_buff, 0, std::size(scissors), scissors);
	vkCmdSetViewport(gfx_cmd_buff, 0, std::size(viewports), viewports);

	const VkBuffer vtx_buffs[] = {
		vk_state.geom_buffer.buffer,
	};

	const VkDeviceSize vtx_buff_offs[] = {
		0,
	};

	vkCmdBindVertexBuffers2(gfx_cmd_buff, 0, std::size(vtx_buffs), vtx_buffs, vtx_buff_offs, nullptr, nullptr);

	const vk_graphics_pipeline::PushConstants gfx_pc = {
		.pos_offset = {
			delta_mouse[0],
			delta_mouse[1],
		},
		.zoom_level = rt_state.zoom_level,
	};

	const VkPushConstantsInfo gfx_pc_info = {
		.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
		.layout = vk_state.gfx_ppln.lyt,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.size = sizeof(vk_graphics_pipeline::PushConstants),
		.pValues = &gfx_pc,
	};

	vkCmdPushConstants2(gfx_cmd_buff, &gfx_pc_info);

	vkCmdDraw(gfx_cmd_buff, 6, 1, 0, 0);

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Awesome Panel");

	if (ImGui::InputInt2("Render Dims", imgui_state.tmp_render_dims))
	{
		imgui_state.tmp_render_dims[0] = std::clamp(imgui_state.tmp_render_dims[0], 1, 8192);
		imgui_state.tmp_render_dims[1] = std::clamp(imgui_state.tmp_render_dims[1], 1, 8192);
	}

	if (ImGui::DragInt("Num Samples", &imgui_state.tmp_max_samples))
	{
		if (imgui_state.tmp_max_samples <= 0)
		{
			imgui_state.tmp_max_samples = 1;
		}
	}

	if (ImGui::Button("Render"))
	{
		imgui_state.should_be_rendering = true;
	}

	ImGui::End();
	ImGui::Render();

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), gfx_cmd_buff);

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
	vkCmdEndRendering(gfx_cmd_buff);

	change_image_layout(gfx_cmd_buff,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		curr_sc_img);

	VK_CHECK("end gfx cmd_buff", vkEndCommandBuffer(gfx_cmd_buff));

	std::vector<VkSemaphoreSubmitInfo> gfx_wait_sem_infos = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = vk_state.swapchain_data.acq_sig_sems[frame_in_flight],
			.stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		},
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = vk_state.swapchain_data.gfx_frame_sems[frame_in_flight],
			.value = vk_state.swapchain_data.gfx_frame_sem_vals[frame_in_flight],
			.stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		}
	};

	const VkCommandBufferSubmitInfo gfx_cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = gfx_cmd_buff,
		},
	};

	const VkSemaphoreSubmitInfo gfx_sig_sem_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = vk_state.swapchain_data.gfx_frame_sems[frame_in_flight],
			.value = ++vk_state.swapchain_data.gfx_frame_sem_vals[frame_in_flight],
			.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		},
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = vk_state.swapchain_data.present_wait_sems[frame_in_flight],
			.stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		},
	};

	const VkSubmitInfo2 gfx_submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = static_cast<uint32_t>(gfx_wait_sem_infos.size()),
			.pWaitSemaphoreInfos = gfx_wait_sem_infos.data(),
			.commandBufferInfoCount = std::size(gfx_cmd_buff_infos),
			.pCommandBufferInfos = gfx_cmd_buff_infos,
			.signalSemaphoreInfoCount = std::size(gfx_sig_sem_infos),
			.pSignalSemaphoreInfos = gfx_sig_sem_infos,
		},
	};

	VK_CHECK("submit drawing commamds", vkQueueSubmit2(vk_state.device_data.gfx_q, std::size(gfx_submit_infos), gfx_submit_infos, VK_NULL_HANDLE));

	const VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vk_state.swapchain_data.present_wait_sems[frame_in_flight],
		.swapchainCount = 1,
		.pSwapchains = &vk_state.swapchain_data.swapchain,
		.pImageIndices = &sc_img_idx,
	};

	VK_CHECK("q present", vkQueuePresentKHR(vk_state.device_data.gfx_q, &present_info));

	vk_state.swapchain_data.gfx_frame_in_flight = (frame_in_flight + 1) % vk_state.swapchain_data.max_frames_in_flight;

	if (imgui_state.should_be_rendering)
	{
		if (static_cast<uint32_t>(imgui_state.tmp_max_samples) <= vk_state.max_samples)
			vk_state.curr_sample = 1;

		vk_state.max_samples = imgui_state.tmp_max_samples;

		if (rt_state.dims.width != imgui_state.tmp_render_dims[0] || rt_state.dims.height != imgui_state.tmp_render_dims[1])
		{
			rt_state.dims.width = imgui_state.tmp_render_dims[0];
			rt_state.dims.height = imgui_state.tmp_render_dims[1];

			should_stop_rendering = true;
			while (is_rendering) {}
			should_stop_rendering = false;

			VK_CHECK("device wait", vkDeviceWaitIdle(device));

			cmpt_swapchain::recreate_render_targets(vk_state.cmpt_swapchain_data, device, { rt_state.dims.width, rt_state.dims.height, 1 }, vk_state.allocator, { vk_state.phy_dev_data.cmpt_q_fly_idx, vk_state.phy_dev_data.gfx_q_fly_idx, vk_state.phy_dev_data.xfer_q_fly_idx }, vk_state.xfer_cmd_pool_data.cmd_buffs[0], vk_state.device_data.xfer_q);

			vk_state.curr_sample = 1;
		}

		imgui_state.should_be_rendering = false;

		if (!is_rendering)
		{
			render_thread = std::thread(render);
			render_thread.detach();
		}
	}

	VK_CHECK("wait gfx q", vkQueueWaitIdle(vk_state.device_data.gfx_q));

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	while (is_rendering) {}

	VkDevice device = vk_state.device_data.device;
	VK_CHECK("device wait idle", vkDeviceWaitIdle(device));

	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplVulkan_Shutdown();
	ImGui::DestroyContext();

	vkDestroyDescriptorPool(device, vk_state.imgui_pool, nullptr);

	vk_buffer::destroy(vk_state.geom_buffer, vk_state.allocator, device);
	vk_graphics_pipeline::destroy(vk_state.gfx_ppln, device);
	vk_command_pool::destroy(vk_state.xfer_cmd_pool_data, device);
	vk_swapchain::destroy(vk_state.swapchain_data, device);
	cmpt_swapchain::destroy(vk_state.cmpt_swapchain_data, vk_state.allocator, device);

	vmaDestroyAllocator(vk_state.allocator);

	vk_device::destroy(device);
	SDL_Vulkan_DestroySurface(vk_state.instance, vk_state.surface_data.surface, nullptr);
	vk_instance::destroy(vk_state.instance);
}