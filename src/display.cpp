#include "display.hpp"
#include "utils.hpp"
#include "frame_objects.hpp"

struct DisplayPipelineData
{
	struct PushConstants
	{
		float PositionOffset[2];
		float ZoomLevel;
	};

	VkPipeline Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;

	VkDevice Device = VK_NULL_HANDLE;
};

DisplayPipelineData DisplayPipelineData_Create(const VulkanInterface& vulkan_interface, const std::string& current_path)
{
	DisplayPipelineData dpd = {
		.Device = vulkan_interface.DeviceData.Device,
	};

	dpd.DescriptorSetLayouts.resize(1);

	Slang::ComPtr<slang::IGlobalSession> slang_global_session;
	SLANG_CHECK("create global session", slang::createGlobalSession(slang_global_session.writeRef()));

	const slang::TargetDesc target_descs[] = {
		{
			.format = SLANG_SPIRV,
			.profile = slang_global_session->findProfile("spirv_1_6"),
		}
	};

#ifdef _DEBUG
	slang::CompilerOptionEntry compiler_options[] = {
		{
			.name = slang::CompilerOptionName::DebugInformation,
			.value = {
				.kind = slang::CompilerOptionValueKind::Int,
				.intValue0 = SLANG_DEBUG_INFO_LEVEL_MAXIMAL,
			}
		},
	};
#endif	// _DEBUG

	slang::SessionDesc session_desc = {
		.targets = target_descs,
		.targetCount = std::size(target_descs),
#ifdef _DEBUG
		.compilerOptionEntries = compiler_options,
		.compilerOptionEntryCount = std::size(compiler_options),
#endif	// _DEBUG
	};

	Slang::ComPtr<slang::ISession> compile_session;
	SLANG_CHECK("create compile session", slang_global_session->createSession(session_desc, compile_session.writeRef()));

	const std::string slang_shader_path = std::string(current_path).append("/shaders/slang/display.slang");

	Slang::ComPtr<slang::IBlob> diagnostic_blob;
	slang::IModule* slang_module = compile_session->loadModule(slang_shader_path.c_str(), diagnostic_blob.writeRef());

	if (diagnostic_blob != nullptr)
	{
		std::println("{}", reinterpret_cast<const char*>(diagnostic_blob->getBufferPointer()));
	}

	Slang::ComPtr<slang::IEntryPoint> vert_entry_point;
	slang_module->findEntryPointByName("vertex_main", vert_entry_point.writeRef());

	std::array<slang::IComponentType*, 2> vert_component_types = { slang_module, vert_entry_point };
	Slang::ComPtr<slang::IComponentType> vert_composed_program;
	diagnostic_blob.setNull();
	SLANG_CHECK("create program", compile_session->createCompositeComponentType(vert_component_types.data(), vert_component_types.size(), vert_composed_program.writeRef(), diagnostic_blob.writeRef()));

	Slang::ComPtr<slang::IComponentType> vert_linked_program;
	diagnostic_blob.setNull();
	SLANG_CHECK("link program", vert_composed_program->link(vert_linked_program.writeRef(), diagnostic_blob.writeRef()));

	Slang::ComPtr<slang::IBlob> vert_spirv_code;
	diagnostic_blob.setNull();
	SLANG_CHECK("get spirv code", vert_composed_program->getEntryPointCode(0, 0, vert_spirv_code.writeRef(), diagnostic_blob.writeRef()));

	const VkShaderModuleCreateInfo vert_mod_ci = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = vert_spirv_code->getBufferSize(),
		.pCode = reinterpret_cast<const uint32_t*>(vert_spirv_code->getBufferPointer()),
	};

	VkShaderModule vert_mod = VK_NULL_HANDLE;
	VK_CHECK("create vert shader module", vkCreateShaderModule(vulkan_interface.DeviceData.Device, &vert_mod_ci, nullptr, &vert_mod));

	Slang::ComPtr<slang::IEntryPoint> frag_entry_point;
	slang_module->findEntryPointByName("fragment_main", frag_entry_point.writeRef());

	std::array<slang::IComponentType*, 2> frag_component_types = { slang_module, frag_entry_point };
	Slang::ComPtr<slang::IComponentType> frag_composed_program;
	diagnostic_blob.setNull();
	SLANG_CHECK("create program", compile_session->createCompositeComponentType(frag_component_types.data(), frag_component_types.size(), frag_composed_program.writeRef(), diagnostic_blob.writeRef()));

	Slang::ComPtr<slang::IComponentType> frag_linked_program;
	diagnostic_blob.setNull();
	SLANG_CHECK("link program", frag_composed_program->link(frag_linked_program.writeRef(), diagnostic_blob.writeRef()));

	Slang::ComPtr<slang::IBlob> frag_spirv_code;
	diagnostic_blob.setNull();
	SLANG_CHECK("get spirv code", frag_composed_program->getEntryPointCode(0, 0, frag_spirv_code.writeRef(), diagnostic_blob.writeRef()));

	const VkShaderModuleCreateInfo frag_mod_ci = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = frag_spirv_code->getBufferSize(),
		.pCode = reinterpret_cast<const uint32_t*>(frag_spirv_code->getBufferPointer()),
	};

	VkShaderModule frag_mod = VK_NULL_HANDLE;
	VK_CHECK("create frag shader module", vkCreateShaderModule(vulkan_interface.DeviceData.Device, &frag_mod_ci, nullptr, &frag_mod));

	const VkPipelineShaderStageCreateInfo stages[] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vert_mod,
			.pName = "main",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = frag_mod,
			.pName = "main",
		},
	};

	const VkVertexInputBindingDescription vbds[] = {
		{
			.binding = 0,
			.stride = sizeof(float) * 4,
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		},
	};

	const VkVertexInputAttributeDescription vads[] = {
		{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
		},
		{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = sizeof(float) * 2,
		},
	};

	const VkPipelineVertexInputStateCreateInfo vis_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = std::size(vbds),
		.pVertexBindingDescriptions = vbds,
		.vertexAttributeDescriptionCount = std::size(vads),
		.pVertexAttributeDescriptions = vads,
	};

	const VkViewport viewports[] = {
		{},
	};

	const VkRect2D scissors[] = {
		{},
	};

	const VkPipelineViewportStateCreateInfo vs_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = std::size(viewports),
		.pViewports = viewports,
		.scissorCount = std::size(scissors),
		.pScissors = scissors,
	};

	const VkPipelineInputAssemblyStateCreateInfo ias_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	};

	const VkPipelineRasterizationStateCreateInfo ras_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
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
		.attachmentCount = std::size(cbas),
		.pAttachments = cbas,
	};

	std::vector<VkDynamicState> ds = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	const VkPipelineDynamicStateCreateInfo ds_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = static_cast<uint32_t>(ds.size()),
		.pDynamicStates = ds.data(),
	};

	const VkDescriptorSetLayoutBinding dsl_binds[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
	};

	const VkDescriptorSetLayoutCreateInfo dsl_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = std::size(dsl_binds),
		.pBindings = dsl_binds,
	};

	VK_CHECK("create dsl", vkCreateDescriptorSetLayout(vulkan_interface.DeviceData.Device, &dsl_ci, nullptr, &dpd.DescriptorSetLayouts[0]));

	const VkPushConstantRange pc_rngs[] = {
		{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.size = sizeof(DisplayPipelineData::PushConstants),
		},
	};

	const VkPipelineLayoutCreateInfo lyt_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &dpd.DescriptorSetLayouts[0],
		.pushConstantRangeCount = std::size(pc_rngs),
		.pPushConstantRanges = pc_rngs,
	};

	VK_CHECK("create graphics pipeline layout", vkCreatePipelineLayout(vulkan_interface.DeviceData.Device, &lyt_ci, nullptr, &dpd.PipelineLayout));

	const VkFormat col_attach_forms[] = {
		VK_FORMAT_R8G8B8A8_UNORM,
	};

	const VkPipelineRenderingCreateInfo rend_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = std::size(col_attach_forms),
		.pColorAttachmentFormats = col_attach_forms,
	};

	const VkGraphicsPipelineCreateInfo cis[] = {
		{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = &rend_info,
			.stageCount = std::size(stages),
			.pStages = stages,
			.pVertexInputState = &vis_ci,
			.pInputAssemblyState = &ias_ci,
			.pViewportState = &vs_ci,
			.pRasterizationState = &ras_ci,
			.pMultisampleState = &ms_ci,
			.pColorBlendState = &cbs_ci,
			.pDynamicState = &ds_ci,
			.layout = dpd.PipelineLayout
		},
	};

	VK_CHECK("create graphics pipeline", vkCreateGraphicsPipelines(vulkan_interface.DeviceData.Device, VK_NULL_HANDLE, std::size(cis), cis, nullptr, &dpd.Pipeline));

	vkDestroyShaderModule(vulkan_interface.DeviceData.Device, vert_mod, nullptr);
	vkDestroyShaderModule(vulkan_interface.DeviceData.Device, frag_mod, nullptr);

	return dpd;
}

void DisplayPipelineData_Destroy(DisplayPipelineData dpd)
{
	if (dpd.Device != VK_NULL_HANDLE)
	{

		for (auto& desc_set_layout : dpd.DescriptorSetLayouts)
			vkDestroyDescriptorSetLayout(dpd.Device, desc_set_layout, nullptr);

		vkDestroyPipeline(dpd.Device, dpd.Pipeline, nullptr);
		vkDestroyPipelineLayout(dpd.Device, dpd.PipelineLayout, nullptr);
	}
}

struct Display
{
	ImageResource FinalRenderTarget = {};

	FrameObjects* FrameObjects = nullptr;
	std::vector<VkSemaphore> PresentWaitSemaphores;
	std::vector<VkSemaphore> AcquireSignalSemaphores;
	std::vector<VkDescriptorSet> DescriptorSets;
	VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;

	DisplayPipelineData PipelineData = {};
	BufferResource GeometryBuffer = {};

	SwapchainData SwapchainData = {};
	VkExtent2D Extent = {};
	VkQueue Queue = VK_NULL_HANDLE;
	VkDevice Device = VK_NULL_HANDLE;

	uint8_t MaxFramesInFlight = 0;
	uint8_t FrameInFlight = 0;
};

Display* Display_Create(const VulkanInterface& vulkan_interface, const std::string& current_path)
{
	Display* d = reinterpret_cast<Display*>(std::calloc(1, sizeof(Display)));

	d->FinalRenderTarget = vulkan_interface.FinalRenderTarget;
	d->MaxFramesInFlight = static_cast<uint8_t>(vulkan_interface.SwapchainData.ImagesCount);
	d->Extent = vulkan_interface.SurfaceData.SurfaceCapabilities.surfaceCapabilities.currentExtent;
	d->SwapchainData = vulkan_interface.SwapchainData;
	d->Queue = vulkan_interface.DeviceData.GraphicsQueue;
	d->Device = vulkan_interface.DeviceData.Device;
	d->FrameObjects = FrameObjects_Create(d->Device, vulkan_interface.PhysicalDeviceData.GraphicsQueueFamilyIndex, d->MaxFramesInFlight);

	d->AcquireSignalSemaphores.resize(d->MaxFramesInFlight);
	d->PresentWaitSemaphores.resize(d->MaxFramesInFlight);
	d->DescriptorSets.resize(d->MaxFramesInFlight);

	const VkSemaphoreTypeCreateInfo sem_type_ci = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
		.initialValue = 0,
	};

	const VkSemaphoreCreateInfo tl_sem_ci = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = &sem_type_ci,
	};

	const VkSemaphoreCreateInfo bin_sem_ci = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	VkSemaphoreSignalInfo sem_sig_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
		.value = 1,
	};

	d->PipelineData = DisplayPipelineData_Create(vulkan_interface, current_path);

	const VkDescriptorPoolSize pool_sizes[] = {
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
		},
	};

	const VkDescriptorPoolCreateInfo dp_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = d->MaxFramesInFlight,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes,
	};

	VK_CHECK("create dsp", vkCreateDescriptorPool(vulkan_interface.DeviceData.Device, &dp_ci, nullptr, &d->DescriptorPool));

	const VkDescriptorSetAllocateInfo ds_ai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = d->DescriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = d->PipelineData.DescriptorSetLayouts.data(),
	};

	for (uint8_t fr = 0; fr < d->MaxFramesInFlight; ++fr)
	{
		VK_CHECK("create acq sig semaphore", vkCreateSemaphore(d->Device, &bin_sem_ci, nullptr, d->AcquireSignalSemaphores.data() + fr));
		VK_CHECK("create present wait semaphore", vkCreateSemaphore(d->Device, &bin_sem_ci, nullptr, d->PresentWaitSemaphores.data() + fr));
		VK_CHECK("allocate display desc sets", vkAllocateDescriptorSets(d->Device, &ds_ai, d->DescriptorSets.data() + fr));
	}

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

	d->GeometryBuffer = BufferResource_Create(vulkan_interface.DeviceData.Device, vulkan_interface.Allocator, verts_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, "geometry buffer");

	BufferResource staging_buffer = BufferResource_Create(vulkan_interface.DeviceData.Device, vulkan_interface.Allocator, verts_size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST, "staging geometry buffer");

	std::memcpy(staging_buffer.AllocationInfo.allocationInfo.pMappedData, verts, verts_size);

	Utils_CopyBufferToBuffer(vulkan_interface.TransferObjects.CommandBuffer, vulkan_interface.TransferObjects.Queue, staging_buffer.DescriptorInfo.buffer, d->GeometryBuffer.DescriptorInfo.buffer, verts_size);

	BufferResource_Destroy(staging_buffer);

	return d;
}

void Display_Destroy(Display* d)
{
	if (d->Device != VK_NULL_HANDLE)
	{
		for (uint8_t fr = 0; fr < d->MaxFramesInFlight; ++fr)
		{
			vkDestroySemaphore(d->Device, d->AcquireSignalSemaphores[fr], nullptr);
			vkDestroySemaphore(d->Device, d->PresentWaitSemaphores[fr], nullptr);
		}
	}

	vkDestroyDescriptorPool(d->Device, d->DescriptorPool, nullptr);
	FrameObjects_Destroy(d->FrameObjects);
	DisplayPipelineData_Destroy(d->PipelineData);
	BufferResource_Destroy(d->GeometryBuffer);

	free(d);
}

void Display_Render(Display* display, const float position_offset[], const float zoom_level, ImGUIState* imgui_state)
{
	VkDevice device = display->Device;
	VkCommandBuffer cmd_buff = FrameObjects_GetCommandBuffer(display->FrameObjects);
	VkSemaphore frame_sem = FrameObjects_GetSemaphore(display->FrameObjects);
	uint64_t& frame_sem_value = FrameObjects_GetFrameSemValue(display->FrameObjects);
	uint8_t frame_in_flight = FrameObjects_GetFrameInFlight(display->FrameObjects);

	const VkSemaphoreWaitInfo wait_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		.semaphoreCount = 1,
		.pSemaphores = &frame_sem,
		.pValues = &frame_sem_value,
	};

	VK_CHECK("wait acq img", vkWaitSemaphores(device, &wait_info, UINT64_MAX));

	const VkAcquireNextImageInfoKHR acq_info = {
		.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
		.swapchain = display->SwapchainData.Swapchain,
		.timeout = UINT64_MAX,
		.semaphore = display->AcquireSignalSemaphores[frame_in_flight],
		.deviceMask = 0x1,
	};

	uint32_t img_idx = 0;
	VK_CHECK("acq img idx", vkAcquireNextImage2KHR(device, &acq_info, &img_idx));

	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	VK_CHECK("begin cmd buff", vkBeginCommandBuffer(cmd_buff, &begin_info));

	Utils_ChangeImageLayout(cmd_buff,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		display->SwapchainData.Images[img_idx]);

	VkRenderingAttachmentInfo col_attachs[] = {
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = display->SwapchainData.ImageViews[img_idx],
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
			.extent = display->Extent,
		},
		.layerCount = 1,
		.colorAttachmentCount = std::size(col_attachs),
		.pColorAttachments = col_attachs,
	};

	vkCmdBeginRendering(cmd_buff, &rendering_info);

	vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, display->PipelineData.Pipeline);

	const VkWriteDescriptorSet desc_writes[] = {
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = display->DescriptorSets[frame_in_flight],
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &display->FinalRenderTarget.DescriptorInfo,
		},
	};

	vkUpdateDescriptorSets(device, std::size(desc_writes), desc_writes, 0, nullptr);

	const VkBindDescriptorSetsInfo bind_desc_sets_info = {
		.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.layout = display->PipelineData.PipelineLayout,
		.descriptorSetCount = 1,
		.pDescriptorSets = &display->DescriptorSets[frame_in_flight],
	};

	vkCmdBindDescriptorSets2(cmd_buff, &bind_desc_sets_info);

	const VkViewport viewports[] = {
		{
			.width = static_cast<float>(display->Extent.width),
			.height = static_cast<float>(display->Extent.height),
			.maxDepth = 1.f,
		},
	};

	const VkRect2D scissors[] = {
		{
			.extent = display->Extent,
		},
	};

	vkCmdSetScissor(cmd_buff, 0, std::size(scissors), scissors);
	vkCmdSetViewport(cmd_buff, 0, std::size(viewports), viewports);

	const VkBuffer vtx_buffs[] = {
		display->GeometryBuffer.DescriptorInfo.buffer,
	};

	const VkDeviceSize vtx_buff_offs[] = {
		0,
	};

	vkCmdBindVertexBuffers2(cmd_buff, 0, std::size(vtx_buffs), vtx_buffs, vtx_buff_offs, nullptr, nullptr);

	const DisplayPipelineData::PushConstants gfx_pc = {
		.PositionOffset = {
			position_offset[0],
			position_offset[1],
		},
		.ZoomLevel = zoom_level,
	};

	const VkPushConstantsInfo gfx_pc_info = {
		.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
		.layout = display->PipelineData.PipelineLayout,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.size = sizeof(DisplayPipelineData::PushConstants),
		.pValues = &gfx_pc,
	};

	vkCmdPushConstants2(cmd_buff, &gfx_pc_info);

	vkCmdDraw(cmd_buff, 6, 1, 0, 0);

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Awesome Panel");

	if (ImGui::InputInt2("Render Dims", imgui_state->TempRenderTargetExtent))
	{
		imgui_state->TempRenderTargetExtent[0] = std::clamp(imgui_state->TempRenderTargetExtent[0], 1, 8192);
		imgui_state->TempRenderTargetExtent[1] = std::clamp(imgui_state->TempRenderTargetExtent[1], 1, 8192);
	}

	if (ImGui::DragInt("Num Samples", &imgui_state->TempMaxSamples))
	{
		if (imgui_state->TempMaxSamples <= 0)
		{
			imgui_state->TempMaxSamples = 1;
		}
	}

	if (ImGui::Button("Render"))
	{
		imgui_state->StartRaytracing = true;
	}

	ImGui::End();
	ImGui::Render();

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd_buff);

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	vkCmdEndRendering(cmd_buff);

	Utils_ChangeImageLayout(cmd_buff,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		display->SwapchainData.Images[img_idx]);

	VK_CHECK("end display cmd buff", vkEndCommandBuffer(cmd_buff));

	const VkSemaphoreSubmitInfo wait_sem_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = display->AcquireSignalSemaphores[frame_in_flight],
			.stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		},
	};

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = cmd_buff,
		},
	};

	const VkSemaphoreSubmitInfo sig_sem_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = display->PresentWaitSemaphores[frame_in_flight],
			.stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		},
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = frame_sem,
			.value = ++frame_sem_value,
			.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		}
	};

	const VkSubmitInfo2 submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.waitSemaphoreInfoCount = std::size(wait_sem_infos),
			.pWaitSemaphoreInfos = wait_sem_infos,
			.commandBufferInfoCount = std::size(cmd_buff_infos),
			.pCommandBufferInfos = cmd_buff_infos,
			.signalSemaphoreInfoCount = std::size(sig_sem_infos),
			.pSignalSemaphoreInfos = sig_sem_infos,
		},
	};

	VK_CHECK("submit display render commands", vkQueueSubmit2(display->Queue, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));

	const VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = display->PresentWaitSemaphores.data() + frame_in_flight,
		.swapchainCount = 1,
		.pSwapchains = &display->SwapchainData.Swapchain,
		.pImageIndices = &img_idx,
	};

	VK_CHECK("q present", vkQueuePresentKHR(display->Queue, &present_info));
	//VK_CHECK("gfx q wait idle", vkQueueWaitIdle(display->Queue));

	FrameObjects_NextFrame(display->FrameObjects);
}

void Display_UpdateFinalRenderTarget(Display* display, const ImageResource FinalRenderTarget)
{
	display->FinalRenderTarget = FinalRenderTarget;
}
