#include "vk_objects.hpp"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

struct pos2d
{
	float x = 0;
	float y = 0;
};

pos2d operator+(const pos2d& lhs, const pos2d& rhs)
{
	return pos2d{ lhs.x + rhs.x, lhs.y + rhs.y };
}

pos2d operator-(const pos2d& lhs, const pos2d& rhs)
{
	return pos2d{ lhs.x - rhs.x, lhs.y - rhs.y };
}

pos2d operator*(const pos2d& lhs, const float multiplier)
{
	return pos2d{ lhs.x * multiplier , lhs.y * multiplier };
}

pos2d operator/(const pos2d& lhs, const float divisor)
{
	return pos2d{ lhs.x / divisor, lhs.y / divisor };
}

pos2d operator/(const pos2d& lhs, const pos2d& rhs)
{
	return pos2d{ lhs.x / rhs.x, lhs.y / rhs.y };
}

void operator +=(pos2d& lhs, const pos2d& rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
}

void operator -=(pos2d& lhs, const pos2d& rhs)
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
}

struct VulkanState
{
	VkInstance instance = VK_NULL_HANDLE;
	VmaAllocator allocator = VK_NULL_HANDLE;
	vk_surface::data surface_data = {};
	vk_phydev::data phy_dev_data = {};
	vk_device::data device_data = {};
	vk_swapchain::data swapchain_data = {};
	vk_command_pool::data xfer_cmd_pool_data = {};
	float clear_color[4] = { 0,0,0,0 };
	VkDescriptorPool imgui_pool = VK_NULL_HANDLE;
	vk_compute_pipeline::data cmpt_ppln = {};
	vk_graphics_pipeline::data gfx_ppln = {};
	vk_buffer::data geom_buffer = {};
	vk_image::data accum_tgt = {};
	vk_image::data final_rndr = {};
};

struct ImGuiState
{
	int tmp_render_dims[2] = { 1280, 720 };
};

struct RenderTargetState
{
	dim2d dims = { 1280, 720 };
	float zoom_level = 1;
	bool is_reset = false;
};

SDL_Window* window = nullptr;
std::string current_path;
VulkanState vk_state = {};
pos2d delta_mouse = {};
RenderTargetState rt_state = {};
pos2d last_mouse_pos = {};
ImGuiState imgui_state = {};
bool mouse_motion_tracking = false;
uint16_t num_samples = 0;

#define SDL_CHECK(result)						\
	if (!result) {									\
		SDL_Log("%s\n", SDL_GetError());		\
		return SDL_APP_FAILURE;					\
	}


SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
	current_path = std::filesystem::path(std::string(argv[0])).parent_path().string();

	SDL_CHECK(SDL_Init(SDL_INIT_VIDEO));
	window = SDL_CreateWindow("Chizen", rt_state.dims.width, rt_state.dims.height, SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

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
	vk_state.swapchain_data = vk_swapchain::create(vk_state.device_data.device, vk_state.surface_data, vk_state.phy_dev_data, VK_NULL_HANDLE, "swapchain");
	vk_state.xfer_cmd_pool_data = vk_command_pool::create(vk_state.device_data.device, vk_state.phy_dev_data.xfer_q_fly_idx, 1, "xfer command pool");

	const VmaAllocatorCreateInfo vma_alloc_ci = {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = vk_state.phy_dev_data.phy_dev,
		.device = vk_state.device_data.device,
		.instance = vk_state.instance,
	};

	VK_CHECK("create vma allocator", vmaCreateAllocator(&vma_alloc_ci, &vk_state.allocator));

	vk_state.accum_tgt = vk_image::create(
		vk_state.device_data.device,
		{ rt_state.dims.width, rt_state.dims.height, 1 },
		VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		vk_state.allocator, 0, "accum target");

	vk_state.final_rndr = vk_image::create(vk_state.device_data.device,
		{ rt_state.dims.width, rt_state.dims.height, 1 },
		VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		vk_state.allocator, 0, "final render");

	vk_state.cmpt_ppln = vk_compute_pipeline::create(vk_state.device_data.device, current_path, vk_state.swapchain_data.max_frames_in_flight, "compute pipeline");
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

	std::vector<VkBufferCopy> regions{ {.size = verts_size} };

	copy_buffer_to_buffer(geom_staging_buffer.buffer, vk_state.geom_buffer.buffer, regions, vk_state.xfer_cmd_pool_data.cmd_buffs[0], vk_state.device_data.xfer_q);
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
	//ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	SDL_CHECK(ImGui_ImplSDL3_InitForVulkan(window));

	ImGui_ImplVulkan_InitInfo imgui_init_info = {
		.Instance = vk_state.instance,
		.PhysicalDevice = vk_state.phy_dev_data.phy_dev,
		.Device = vk_state.device_data.device,
		.Queue = vk_state.device_data.gfx_q,
		.DescriptorPool = vk_state.imgui_pool,
		.MinImageCount = vk_state.swapchain_data.max_frames_in_flight,
		.ImageCount = vk_state.swapchain_data.max_frames_in_flight,
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
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
	{
		if (!io.WantCaptureMouse)
		{
			mouse_motion_tracking = true;
			last_mouse_pos = { event->motion.x, event->motion.y };
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
	{
		if (!io.WantCaptureMouse)
		{
			mouse_motion_tracking = false;
			rt_state.is_reset = false;
			last_mouse_pos = {};
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_MOTION)
	{
		if (!io.WantCaptureMouse)
		{
			if (mouse_motion_tracking)
			{
				delta_mouse += ((last_mouse_pos - pos2d{ event->motion.x, event->motion.y }) /
					pos2d(static_cast<float>(vk_state.surface_data.surf_caps.currentExtent.width), static_cast<float>(vk_state.surface_data.surf_caps.currentExtent.height))) * 2;

				last_mouse_pos = { event->motion.x, event->motion.y };
				rt_state.is_reset = 1;
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
		vk_swapchain::destroy(vk_state.swapchain_data, vk_state.device_data.device);
		vk_state.swapchain_data = vk_swapchain::create(vk_state.device_data.device, vk_state.surface_data, vk_state.phy_dev_data, VK_NULL_HANDLE, "swapchain");
	}

	return SDL_APP_CONTINUE;
}

bool is_new_render_targets_required()
{
	return (vk_state.accum_tgt.dims.width != rt_state.dims.width) || (vk_state.accum_tgt.dims.height != rt_state.dims.height);
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
	{
		return SDL_APP_CONTINUE;
	}

	VkDevice device = vk_state.device_data.device;

	if (is_new_render_targets_required())
	{
		VK_CHECK("device wait for render target destroy", vkDeviceWaitIdle(device));

			vk_image::destroy(vk_state.accum_tgt, vk_state.allocator, device);
			vk_state.accum_tgt = vk_image::create(
				vk_state.device_data.device,
				{ rt_state.dims.width, rt_state.dims.height, 1 },
				VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				vk_state.allocator, 0, "render target");
	}

	uint8_t curr_frame = vk_state.swapchain_data.curr_frame;

	VK_CHECK("wait for rndr fnc", vkWaitForFences(device, 1, &vk_state.swapchain_data.rndr_fncs[curr_frame], VK_TRUE, UINT64_MAX));

	VkCommandBuffer curr_cmd_buff = vk_state.swapchain_data.cmd_buffs[curr_frame];

	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VK_CHECK("begin sc cmd_buff", vkBeginCommandBuffer(curr_cmd_buff, &begin_info));

	change_image_layout(curr_cmd_buff,
		0, 0,
		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		vk_state.accum_tgt.image);

	change_image_layout(curr_cmd_buff,
		0, 0,
		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		vk_state.final_rndr.image);

	vkCmdBindPipeline(curr_cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, vk_state.cmpt_ppln.pipe);

	vk_state.accum_tgt.desc_img_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	const VkWriteDescriptorSet cmpt_desc_writes[] = {
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = vk_state.cmpt_ppln.dss[curr_frame],
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.pImageInfo = &vk_state.accum_tgt.desc_img_info,
		},
	};

	vkUpdateDescriptorSets(device, std::size(cmpt_desc_writes), cmpt_desc_writes, 0, nullptr);

	const VkBindDescriptorSetsInfo cmpt_ds_bi = {
		.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.layout = vk_state.cmpt_ppln.lyt,
		.descriptorSetCount = 1,
		.pDescriptorSets = &vk_state.cmpt_ppln.dss[curr_frame],
	};

	vkCmdBindDescriptorSets2(curr_cmd_buff, &cmpt_ds_bi);

	const vk_compute_pipeline::PushConstants cmpt_pc = {
		.dispatch_x = rt_state.dims.width,
		.dispatch_y = rt_state.dims.height,
		.current_time = static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count()),
		.num_samples_is_reset = static_cast<uint32_t>((rt_state.is_reset & 0x1) | (++num_samples << 1)),
	};

	const VkPushConstantsInfo cmpt_pc_info = {
		.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
		.layout = vk_state.cmpt_ppln.lyt,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.size = sizeof(vk_compute_pipeline::PushConstants),
		.pValues = &cmpt_pc,
	};

	vkCmdPushConstants2(curr_cmd_buff, &cmpt_pc_info);

	vkCmdDispatch(curr_cmd_buff, cmpt_pc.dispatch_x / 32 + 1, cmpt_pc.dispatch_y / 32 + 1, 1);

	change_image_layout(curr_cmd_buff,
		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
		VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		vk_state.accum_tgt.image
	);

	const VkAcquireNextImageInfoKHR acq_info = {
		.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
		.swapchain = vk_state.swapchain_data.swapchain,
		.timeout = UINT64_MAX,
		.semaphore = vk_state.swapchain_data.acq_sig_sems[curr_frame],
		.deviceMask = 0x1,
	};

	uint32_t sc_img_idx = 0;
	VK_CHECK("acq img idx", vkAcquireNextImage2KHR(device, &acq_info, &sc_img_idx));

	VK_CHECK("reset rndr fnc", vkResetFences(device, 1, &vk_state.swapchain_data.rndr_fncs[curr_frame]));

	VkImage curr_sc_img = vk_state.swapchain_data.images[sc_img_idx];
	VkFence curr_rndr_fnc = vk_state.swapchain_data.rndr_fncs[curr_frame];

	change_image_layout(curr_cmd_buff,
		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		curr_sc_img);

	VkRenderingAttachmentInfo col_attachs[] = {
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = vk_state.swapchain_data.image_views[sc_img_idx],
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_NONE,
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

	vkCmdBeginRendering(curr_cmd_buff, &rendering_info);

	vkCmdBindPipeline(curr_cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_state.gfx_ppln.pipe);

	const VkBindDescriptorSetsInfo gfx_ds_bi = {
		.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.layout = vk_state.gfx_ppln.lyt,
		.descriptorSetCount = 1,
		.pDescriptorSets = &vk_state.gfx_ppln.dss[curr_frame],
	};

	vk_state.accum_tgt.desc_img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	const VkWriteDescriptorSet gfx_desc_writes[] = {
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = vk_state.gfx_ppln.dss[curr_frame],
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &vk_state.accum_tgt.desc_img_info,
		},
	};

	vkUpdateDescriptorSets(device, std::size(gfx_desc_writes), gfx_desc_writes, 0, nullptr);
	vkCmdBindDescriptorSets2(curr_cmd_buff, &gfx_ds_bi);

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

	vkCmdSetScissor(curr_cmd_buff, 0, std::size(scissors), scissors);
	vkCmdSetViewport(curr_cmd_buff, 0, std::size(viewports), viewports);

	const VkBuffer vtx_buffs[] = {
		vk_state.geom_buffer.buffer,
	};

	const VkDeviceSize vtx_buff_offs[] = {
		0,
	};

	vkCmdBindVertexBuffers2(curr_cmd_buff, 0, std::size(vtx_buffs), vtx_buffs, vtx_buff_offs, nullptr, nullptr);

	const vk_graphics_pipeline::PushConstants gfx_pc = {
		.pos_offset = {
			delta_mouse.x,
			delta_mouse.y,
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

	vkCmdPushConstants2(curr_cmd_buff, &gfx_pc_info);

	vkCmdDraw(curr_cmd_buff, 6, 1, 0, 0);

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Awesome Panel");

	if (ImGui::InputInt2("Render Dims", imgui_state.tmp_render_dims))
	{
		imgui_state.tmp_render_dims[0] = std::clamp(imgui_state.tmp_render_dims[0], 1, 8192);
		rt_state.dims.width = imgui_state.tmp_render_dims[0];

		imgui_state.tmp_render_dims[1] = std::clamp(imgui_state.tmp_render_dims[1], 1, 8192);
		rt_state.dims.height = imgui_state.tmp_render_dims[1];

		rt_state.is_reset = true;
	}

	ImGui::End();
	ImGui::Render();

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), curr_cmd_buff);

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
			.semaphore = vk_state.swapchain_data.acq_sig_sems[curr_frame],
			.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		},
	};

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = curr_cmd_buff,
		},
	};

	const VkSemaphoreSubmitInfo sig_sem_info[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = vk_state.swapchain_data.rndr_sems[curr_frame],
			.stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		},
	};

	const VkSubmitInfo2 submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = std::size(wait_sem_infos),
			.pWaitSemaphoreInfos = wait_sem_infos,
			.commandBufferInfoCount = std::size(cmd_buff_infos),
			.pCommandBufferInfos = cmd_buff_infos,
			.signalSemaphoreInfoCount = std::size(sig_sem_info),
			.pSignalSemaphoreInfos = sig_sem_info,
		},
	};

	VK_CHECK("submit drawing commamds", vkQueueSubmit2(vk_state.device_data.gfx_q, std::size(submit_infos), submit_infos, curr_rndr_fnc));

	const VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vk_state.swapchain_data.rndr_sems[curr_frame],
		.swapchainCount = 1,
		.pSwapchains = &vk_state.swapchain_data.swapchain,
		.pImageIndices = &sc_img_idx,
	};

	VK_CHECK("q present", vkQueuePresentKHR(vk_state.device_data.gfx_q, &present_info));

	vk_state.swapchain_data.curr_frame = (curr_frame + 1) % vk_state.swapchain_data.max_frames_in_flight;

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	VkDevice device = vk_state.device_data.device;
	VK_CHECK("device wait idle", vkDeviceWaitIdle(device));

	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplVulkan_Shutdown();
	ImGui::DestroyContext();

	vkDestroyDescriptorPool(device, vk_state.imgui_pool, nullptr);

	vk_buffer::destroy(vk_state.geom_buffer, vk_state.allocator, device);
	vk_graphics_pipeline::destroy(vk_state.gfx_ppln, device);
	vk_compute_pipeline::destroy(vk_state.cmpt_ppln, device);
	vk_command_pool::destroy(vk_state.xfer_cmd_pool_data, device);
	vk_swapchain::destroy(vk_state.swapchain_data, device);

	vk_image::destroy(vk_state.final_rndr, vk_state.allocator, device);
	vk_image::destroy(vk_state.accum_tgt, vk_state.allocator, device);

	vmaDestroyAllocator(vk_state.allocator);

	vk_device::destroy(device);
	SDL_Vulkan_DestroySurface(vk_state.instance, vk_state.surface_data.surface, nullptr);
	vk_instance::destroy(vk_state.instance);
}