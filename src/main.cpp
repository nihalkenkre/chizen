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
	vk_image::data rndr_tgt = {};
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

struct AppState
{
	SDL_Window* window = nullptr;
	std::string current_path;
	VulkanState vk_state = {};
	pos2d delta_mouse = {};
	RenderTargetState rt_state = {};
	pos2d last_mouse_pos = {};
	ImGuiState imgui_state = {};
	bool mouse_motion_tracking = false;
};

#define SDL_CHECK(result)						\
	if (!result) {									\
		SDL_Log("%s\n", SDL_GetError());		\
		return SDL_APP_FAILURE;					\
	}

static AppState app_state = {};

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
	app_state.current_path = std::filesystem::path(std::string(argv[0])).parent_path().string();

	SDL_CHECK(SDL_Init(SDL_INIT_VIDEO));
	app_state.window = SDL_CreateWindow("Chizen", app_state.rt_state.dims.width, app_state.rt_state.dims.height, SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

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
	app_state.vk_state.device_data = vk_device::create(app_state.vk_state.phy_dev_data);
	app_state.vk_state.swapchain_data = vk_swapchain::create(app_state.vk_state.device_data.device, app_state.vk_state.surface_data, app_state.vk_state.phy_dev_data, VK_NULL_HANDLE, "swapchain");
	app_state.vk_state.xfer_cmd_pool_data = vk_command_pool::create(app_state.vk_state.device_data.device, app_state.vk_state.phy_dev_data.xfer_q_fly_idx, 1, "xfer command pool");

	const VmaAllocatorCreateInfo vma_alloc_ci = {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = app_state.vk_state.phy_dev_data.phy_dev,
		.device = app_state.vk_state.device_data.device,
		.instance = app_state.vk_state.instance,
	};

	VK_CHECK("create vma allocator", vmaCreateAllocator(&vma_alloc_ci, &app_state.vk_state.allocator));

	app_state.vk_state.rndr_tgt = vk_image::create(
		app_state.vk_state.device_data.device,
		{ app_state.rt_state.dims.width, app_state.rt_state.dims.height, 1 },
		VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		app_state.vk_state.allocator, "render target");
	app_state.vk_state.cmpt_ppln = vk_compute_pipeline::create(app_state.vk_state.device_data.device, app_state.current_path, "compute pipeline");
	app_state.vk_state.gfx_ppln = vk_graphics_pipeline::create(app_state.vk_state.device_data.device, app_state.current_path, app_state.vk_state.surface_data.format.format, "graphics pipeline");

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

	app_state.vk_state.geom_buffer = vk_buffer::create(app_state.vk_state.device_data.device, app_state.vk_state.allocator, verts_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, "geom buffer");
	vk_buffer::data geom_staging_buffer = vk_buffer::create(app_state.vk_state.device_data.device, app_state.vk_state.allocator, verts_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, "staging geom buffer");

	memcpy(geom_staging_buffer.alloc_info.pMappedData, verts, verts_size);

	std::vector<VkBufferCopy> regions{ {.size = verts_size} };

	copy_buffer_to_buffer(geom_staging_buffer.buffer, app_state.vk_state.geom_buffer.buffer, regions, app_state.vk_state.xfer_cmd_pool_data.cmd_buffs[0], app_state.vk_state.device_data.xfer_q);
	vk_buffer::destroy(geom_staging_buffer, app_state.vk_state.allocator, app_state.vk_state.device_data.device);

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
	//ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
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

	ImGuiIO& io = ImGui::GetIO();

	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
	{
		if (!io.WantCaptureMouse)
		{
			app_state.mouse_motion_tracking = true;
			app_state.last_mouse_pos = { event->motion.x, event->motion.y };
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
	{
		if (!io.WantCaptureMouse)
		{
			app_state.mouse_motion_tracking = false;
			app_state.rt_state.is_reset = false;
			app_state.last_mouse_pos = {};
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_MOTION)
	{
		if (!io.WantCaptureMouse)
		{
			if (app_state.mouse_motion_tracking)
			{
				app_state.delta_mouse += ((app_state.last_mouse_pos - pos2d{ event->motion.x, event->motion.y }) /
					pos2d(static_cast<float>(app_state.vk_state.surface_data.surf_caps.currentExtent.width), static_cast<float>(app_state.vk_state.surface_data.surf_caps.currentExtent.height))) * 2;

				app_state.last_mouse_pos = { event->motion.x, event->motion.y };
				app_state.rt_state.is_reset = 1;
			}
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_WHEEL)
	{
		if (!io.WantCaptureMouse)
		{
			app_state.rt_state.zoom_level = std::max(0.01f, app_state.rt_state.zoom_level + event->wheel.y / 20.f);
		}
	}

	return SDL_APP_CONTINUE;
}

bool is_new_render_target_required()
{
	return (app_state.vk_state.rndr_tgt.dims.width != app_state.rt_state.dims.width) || (app_state.vk_state.rndr_tgt.dims.height != app_state.rt_state.dims.height);
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	if (SDL_GetWindowFlags(app_state.window) & SDL_WINDOW_MINIMIZED)
	{
		return SDL_APP_CONTINUE;
	}

	VkDevice device = app_state.vk_state.device_data.device;

	if (is_new_render_target_required())
	{
		VK_CHECK("device wait for render target destroy", vkDeviceWaitIdle(device));

		vk_image::destroy(app_state.vk_state.rndr_tgt, app_state.vk_state.allocator, device);
		app_state.vk_state.rndr_tgt = vk_image::create(
			app_state.vk_state.device_data.device,
			{ app_state.rt_state.dims.width, app_state.rt_state.dims.height, 1 },
			VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			app_state.vk_state.allocator, "render target");
	}

	const VkAcquireNextImageInfoKHR acq_info = {
		.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
		.swapchain = app_state.vk_state.swapchain_data.swapchain,
		.semaphore = app_state.vk_state.swapchain_data.acq_sig_sem.semaphore,
		.deviceMask = 0x1,
	};

	VkResult result = vkAcquireNextImage2KHR(device, &acq_info, &app_state.vk_state.swapchain_data.curr_img_idx);
	if (result == VK_NOT_READY)
	{
		return SDL_APP_CONTINUE;
	}
	else if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		VK_CHECK("gfx q wait idle", vkQueueWaitIdle(app_state.vk_state.device_data.gfx_q));
		VK_CHECK("get surface capabilities", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app_state.vk_state.phy_dev_data.phy_dev, app_state.vk_state.surface_data.surface, &app_state.vk_state.surface_data.surf_caps));

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
		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		app_state.vk_state.rndr_tgt.image);

	vkCmdBindPipeline(curr_cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, app_state.vk_state.cmpt_ppln.pipe);

	app_state.vk_state.rndr_tgt.desc_img_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	const VkWriteDescriptorSet cmpt_desc_writes[] = {
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = app_state.vk_state.cmpt_ppln.ds,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.pImageInfo = &app_state.vk_state.rndr_tgt.desc_img_info,
		},
	};

	vkUpdateDescriptorSets(device, std::size(cmpt_desc_writes), cmpt_desc_writes, 0, nullptr);

	const VkBindDescriptorSetsInfo cmpt_ds_bi = {
		.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.layout = app_state.vk_state.cmpt_ppln.lyt,
		.descriptorSetCount = 1,
		.pDescriptorSets = &app_state.vk_state.cmpt_ppln.ds,
	};

	vkCmdBindDescriptorSets2(curr_cmd_buff, &cmpt_ds_bi);

	const vk_compute_pipeline::PushConstants cmpt_pc = {
		.dispatch_x = app_state.rt_state.dims.width,
		.dispatch_y = app_state.rt_state.dims.height,
		.current_time = static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count()),
		.is_reset = app_state.rt_state.is_reset,
	};

	const VkPushConstantsInfo cmpt_pc_info = {
		.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
		.layout = app_state.vk_state.cmpt_ppln.lyt,
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
		app_state.vk_state.rndr_tgt.image
	);

	change_image_layout(curr_cmd_buff,
		VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		curr_sc_img);

	VkRenderingAttachmentInfo col_attachs[] = {
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = app_state.vk_state.swapchain_data.image_views[app_state.vk_state.swapchain_data.curr_img_idx],
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
			.extent = app_state.vk_state.surface_data.surf_caps.currentExtent,
		},
		.layerCount = 1,
		.colorAttachmentCount = std::size(col_attachs),
		.pColorAttachments = col_attachs,
	};

	vkCmdBeginRendering(curr_cmd_buff, &rendering_info);

	vkCmdBindPipeline(curr_cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, app_state.vk_state.gfx_ppln.pipe);

	const VkBindDescriptorSetsInfo gfx_ds_bi = {
		.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.layout = app_state.vk_state.gfx_ppln.lyt,
		.descriptorSetCount = 1,
		.pDescriptorSets = &app_state.vk_state.gfx_ppln.ds,
	};

	app_state.vk_state.rndr_tgt.desc_img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	const VkWriteDescriptorSet gfx_desc_writes[] = {
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = app_state.vk_state.gfx_ppln.ds,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &app_state.vk_state.rndr_tgt.desc_img_info,
		},
	};

	vkUpdateDescriptorSets(device, std::size(gfx_desc_writes), gfx_desc_writes, 0, nullptr);
	vkCmdBindDescriptorSets2(curr_cmd_buff, &gfx_ds_bi);

	const VkViewport viewports[] = {
		{
			.width = static_cast<float>(app_state.vk_state.surface_data.surf_caps.currentExtent.width),
			.height = static_cast<float>(app_state.vk_state.surface_data.surf_caps.currentExtent.height),
			.maxDepth = 1.f,
		},
	};

	const VkRect2D scissors[] = {
		{
			.extent = app_state.vk_state.surface_data.surf_caps.currentExtent,
		},
	};

	vkCmdSetScissor(curr_cmd_buff, 0, std::size(scissors), scissors);
	vkCmdSetViewport(curr_cmd_buff, 0, std::size(viewports), viewports);

	const VkBuffer vtx_buffs[] = {
		app_state.vk_state.geom_buffer.buffer,
	};

	const VkDeviceSize vtx_buff_offs[] = {
		0,
	};

	vkCmdBindVertexBuffers2(curr_cmd_buff, 0, std::size(vtx_buffs), vtx_buffs, vtx_buff_offs, nullptr, nullptr);

	const vk_graphics_pipeline::PushConstants gfx_pc = {
		.pos_offset = {
			app_state.delta_mouse.x,
			app_state.delta_mouse.y,
		},
		.zoom_level = app_state.rt_state.zoom_level,
	};

	const VkPushConstantsInfo gfx_pc_info = {
		.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
		.layout = app_state.vk_state.gfx_ppln.lyt,
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

	if (ImGui::InputInt2("Render Dims", app_state.imgui_state.tmp_render_dims))
	{
		app_state.imgui_state.tmp_render_dims[0] = std::clamp(app_state.imgui_state.tmp_render_dims[0], 1, 8192);
		app_state.rt_state.dims.width = app_state.imgui_state.tmp_render_dims[0];

		app_state.imgui_state.tmp_render_dims[1] = std::clamp(app_state.imgui_state.tmp_render_dims[1], 1, 8192);
		app_state.rt_state.dims.height = app_state.imgui_state.tmp_render_dims[1];
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
			.semaphore = app_state.vk_state.swapchain_data.acq_sig_sem.semaphore,
			.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
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
			.waitSemaphoreInfoCount = std::size(wait_sem_infos),
			.pWaitSemaphoreInfos = wait_sem_infos,
			.commandBufferInfoCount = std::size(cmd_buff_infos),
			.pCommandBufferInfos = cmd_buff_infos,
		},
	};

	VK_CHECK("submit drawing commamds", vkQueueSubmit2(app_state.vk_state.device_data.gfx_q, std::size(submit_infos), submit_infos, curr_rndr_fnc));

	VK_CHECK("wait for rndr fnc", vkWaitForFences(device, 1, &curr_rndr_fnc, VK_TRUE, UINT64_MAX));
	VK_CHECK("reset rndr fnc", vkResetFences(device, 1, &curr_rndr_fnc));

	const VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.swapchainCount = 1,
		.pSwapchains = &app_state.vk_state.swapchain_data.swapchain,
		.pImageIndices = &app_state.vk_state.swapchain_data.curr_img_idx,
	};

	result = vkQueuePresentKHR(app_state.vk_state.device_data.gfx_q, &present_info);

	if (result == VK_NOT_READY)
	{
		return SDL_APP_CONTINUE;
	}
	else if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		VK_CHECK("gfx q wait idle", vkQueueWaitIdle(app_state.vk_state.device_data.gfx_q));
		VK_CHECK("get surface capabilities", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app_state.vk_state.phy_dev_data.phy_dev, app_state.vk_state.surface_data.surface, &app_state.vk_state.surface_data.surf_caps));

		vk_swapchain::destroy(app_state.vk_state.swapchain_data, app_state.vk_state.device_data.device);
		app_state.vk_state.swapchain_data = vk_swapchain::create(app_state.vk_state.device_data.device, app_state.vk_state.surface_data, app_state.vk_state.phy_dev_data, VK_NULL_HANDLE, "swapchain");

		return SDL_APP_CONTINUE;
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	VkDevice device = app_state.vk_state.device_data.device;
	VK_CHECK("device wait idle", vkDeviceWaitIdle(device));

	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplVulkan_Shutdown();
	ImGui::DestroyContext();

	vkDestroyDescriptorPool(device, app_state.vk_state.imgui_pool, nullptr);

	vk_buffer::destroy(app_state.vk_state.geom_buffer, app_state.vk_state.allocator, device);
	vk_graphics_pipeline::destroy(app_state.vk_state.gfx_ppln, device);
	vk_compute_pipeline::destroy(app_state.vk_state.cmpt_ppln, device);
	vk_command_pool::destroy(app_state.vk_state.xfer_cmd_pool_data, device);
	vk_swapchain::destroy(app_state.vk_state.swapchain_data, device);
	vk_image::destroy(app_state.vk_state.rndr_tgt, app_state.vk_state.allocator, device);

	vmaDestroyAllocator(app_state.vk_state.allocator);

	vk_device::destroy(device);
	SDL_Vulkan_DestroySurface(app_state.vk_state.instance, app_state.vk_state.surface_data.surface, nullptr);
	vk_instance::destroy(app_state.vk_state.instance);
}