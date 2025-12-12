#include "rasterizer.hpp"
#include "vulkan_interface.hpp"
#include "vulkan_objects.hpp"
#include "frame_objects.hpp"
#include "imgui_state.hpp"
#include "resources.hpp"
#include "scene.hpp"

#include "utils.hpp"

class RasterizerPipelineData
{
public:
	RasterizerPipelineData() = delete;
	RasterizerPipelineData(const VulkanInterface* vulkan_interface, const std::string& current_path, const std::string& name);

	RasterizerPipelineData(const RasterizerPipelineData& other) = delete;
	RasterizerPipelineData& operator=(const RasterizerPipelineData& other) = delete;

	~RasterizerPipelineData() noexcept;

	struct PushConstants
	{

	};

	VkPipeline GetPipeline() const;
	VkPipelineLayout GetPipelineLayout() const;
	const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const;

private:
	VkPipeline mPipeline = VK_NULL_HANDLE;
	VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;

	VkDevice mDevice = VK_NULL_HANDLE;
};

RasterizerPipelineData::RasterizerPipelineData(const VulkanInterface* vulkan_interface, const std::string& current_path, const std::string& name) : mDevice(vulkan_interface->GetDevice()->GetDevice())
{
	//	Slang::ComPtr<slang::IGlobalSession> slang_global_session;
	//	SLANG_CHECK("create global session", slang::createGlobalSession(slang_global_session.writeRef()));
	//
	//	const slang::TargetDesc target_descs[] = {
	//		{
	//			.format = SLANG_SPIRV,
	//			.profile = slang_global_session->findProfile("spirv_1_1"),
	//		}
	//	};
	//
	//	slang::CompilerOptionEntry compiler_options[] = {
	//		{
	//			.name = slang::CompilerOptionName::MatrixLayoutColumn,
	//			.value = {
	//				.kind = slang::CompilerOptionValueKind::Int,
	//				.intValue0 = 1,
	//			},
	//		},
	//#ifdef _DEBUG
	//		{
	//			.name = slang::CompilerOptionName::DebugInformation,
	//			.value = {
	//				.kind = slang::CompilerOptionValueKind::Int,
	//				.intValue0 = SLANG_DEBUG_INFO_LEVEL_MAXIMAL,
	//			}
	//		},
	//#endif	// _DEBUG
	//	};
	//
	//	slang::SessionDesc session_desc = {
	//		.targets = target_descs,
	//		.targetCount = std::size(target_descs),
	//		.compilerOptionEntries = compiler_options,
	//		.compilerOptionEntryCount = std::size(compiler_options),
	//	};
	//
	//	Slang::ComPtr<slang::ISession> compile_session;
	//	SLANG_CHECK("create compile session", slang_global_session->createSession(session_desc, compile_session.writeRef()));
	//
	//	const std::string slang_shader_path = std::string(current_path).append("/shaders/slang/rasterize.slang");
	//
	//	Slang::ComPtr<slang::IBlob> diagnostic_blob;
	//	slang::IModule* slang_module = compile_session->loadModule(slang_shader_path.c_str(), diagnostic_blob.writeRef());
	//
	//	if (diagnostic_blob != nullptr)
	//	{
	//		std::println("{}", reinterpret_cast<const char*>(diagnostic_blob->getBufferPointer()));
	//	}
	//
	//	Slang::ComPtr<slang::IEntryPoint> vert_entry_point;
	//	slang_module->findEntryPointByName("vertex_main", vert_entry_point.writeRef());
	//
	//	std::array<slang::IComponentType*, 2> vert_component_types = { slang_module, vert_entry_point };
	//	Slang::ComPtr<slang::IComponentType> vert_composed_program;
	//	diagnostic_blob.setNull();
	//	SLANG_CHECK("create program", compile_session->createCompositeComponentType(vert_component_types.data(), vert_component_types.size(), vert_composed_program.writeRef(), diagnostic_blob.writeRef()));
	//
	//	Slang::ComPtr<slang::IComponentType> vert_linked_program;
	//	diagnostic_blob.setNull();
	//	SLANG_CHECK("link program", vert_composed_program->link(vert_linked_program.writeRef(), diagnostic_blob.writeRef()));
	//
	//	Slang::ComPtr<slang::IBlob> vert_spirv_code;
	//	diagnostic_blob.setNull();
	//	SLANG_CHECK("get spirv code", vert_composed_program->getEntryPointCode(0, 0, vert_spirv_code.writeRef(), diagnostic_blob.writeRef()));
	//
	//	const VkShaderModuleCreateInfo vert_mod_ci = {
	//		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	//		.codeSize = vert_spirv_code->getBufferSize(),
	//		.pCode = reinterpret_cast<const uint32_t*>(vert_spirv_code->getBufferPointer()),
	//	};
	//
	//	VkShaderModule vert_mod = VK_NULL_HANDLE;
	//	VK_CHECK("create vert shader module", vkCreateShaderModule(vulkan_interface->GetDevice()->GetDevice(), &vert_mod_ci, nullptr, &vert_mod));
	//
	//	Slang::ComPtr<slang::IEntryPoint> frag_entry_point;
	//	slang_module->findEntryPointByName("fragment_main", frag_entry_point.writeRef());
	//
	//	std::array<slang::IComponentType*, 2> frag_component_types = { slang_module, frag_entry_point };
	//	Slang::ComPtr<slang::IComponentType> frag_composed_program;
	//	diagnostic_blob.setNull();
	//	SLANG_CHECK("create program", compile_session->createCompositeComponentType(frag_component_types.data(), frag_component_types.size(), frag_composed_program.writeRef(), diagnostic_blob.writeRef()));
	//
	//	Slang::ComPtr<slang::IComponentType> frag_linked_program;
	//	diagnostic_blob.setNull();
	//	SLANG_CHECK("link program", frag_composed_program->link(frag_linked_program.writeRef(), diagnostic_blob.writeRef()));
	//
	//	Slang::ComPtr<slang::IBlob> frag_spirv_code;
	//	diagnostic_blob.setNull();
	//	SLANG_CHECK("get spirv code", frag_composed_program->getEntryPointCode(0, 0, frag_spirv_code.writeRef(), diagnostic_blob.writeRef()));
	//
	//	const VkShaderModuleCreateInfo frag_mod_ci = {
	//		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	//		.codeSize = frag_spirv_code->getBufferSize(),
	//		.pCode = reinterpret_cast<const uint32_t*>(frag_spirv_code->getBufferPointer()),
	//	};
	//
	//	VkShaderModule frag_mod = VK_NULL_HANDLE;
	//	VK_CHECK("create frag shader module", vkCreateShaderModule(vulkan_interface->GetDevice()->GetDevice(), &frag_mod_ci, nullptr, &frag_mod));

	std::filesystem::path vert_path = std::string(current_path).append("/shaders/glsl/rasterizer.vert.glsl.spv");
	VkShaderModule vert_mod = VK_NULL_HANDLE;
	if (std::filesystem::exists(vert_path))
	{
		std::uintmax_t file_size = std::filesystem::file_size(vert_path);
		std::ifstream vert_file(vert_path.c_str(), std::ios::binary);

		std::vector<char> vert_code(file_size, 0);
		vert_file.read(vert_code.data(), file_size);

		const VkShaderModuleCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = file_size,
			.pCode = reinterpret_cast<uint32_t*>(vert_code.data()),
		};
		VK_CHECK("create rgen module", vkCreateShaderModule(mDevice, &ci, nullptr, &vert_mod));
	}
	else
	{
		std::println("Could not find {}", vert_path.string());
	}

	std::filesystem::path frag_path = std::string(current_path).append("/shaders/glsl/rasterizer.frag.glsl.spv");
	VkShaderModule frag_mod = VK_NULL_HANDLE;
	if (std::filesystem::exists(frag_path))
	{
		std::uintmax_t file_size = std::filesystem::file_size(frag_path);
		std::ifstream frag_file(frag_path.c_str(), std::ios::binary);

		std::vector<char> frag_code(file_size, 0);
		frag_file.read(frag_code.data(), file_size);

		const VkShaderModuleCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = file_size,
			.pCode = reinterpret_cast<uint32_t*>(frag_code.data()),
		};
		VK_CHECK("create frag module", vkCreateShaderModule(mDevice, &ci, nullptr, &frag_mod));
	}
	else
	{
		std::println("Could not find {}", frag_path.string());
	}

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

	const VkVertexInputBindingDescription vibds[] = {
		{
			.binding = 0,
			.stride = sizeof(float) * 3,
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		},
		{
			.binding = 1,
			.stride = sizeof(float) * 2,
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		},
		{
			.binding = 2,
			.stride = sizeof(float) * 3,
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		}
	};

	const VkVertexInputAttributeDescription vads[] = {
		{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
		},
		{
			.location = 1,
			.binding = 1,
			.format = VK_FORMAT_R32G32_SFLOAT,
		},
		{
			.location = 2,
			.binding = 2,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
		},
	};

	const VkPipelineVertexInputStateCreateInfo vis_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = std::size(vibds),
		.pVertexBindingDescriptions = vibds,
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
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
	};

	const VkPipelineMultisampleStateCreateInfo ms_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};

	const VkPipelineDepthStencilStateCreateInfo dss = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
		.minDepthBounds = 0.f,
		.maxDepthBounds = 1.f,
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

	const VkDynamicState ds[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	const VkPipelineDynamicStateCreateInfo ds_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = _countof(ds),
		.pDynamicStates = ds,
	};

	const VkDescriptorSetLayoutBinding dsl_0_binds[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		}
	};

	const VkDescriptorSetLayoutBinding dsl_1_binds[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		},
		//{
		//	.binding = 1,
		//	.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		//	.descriptorCount = 1,
		//	.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		//},
	};

	const VkDescriptorSetLayoutCreateInfo dsl_0_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = std::size(dsl_0_binds),
		.pBindings = dsl_0_binds,
	};

	const VkDescriptorSetLayoutCreateInfo dsl_1_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = std::size(dsl_1_binds),
		.pBindings = dsl_1_binds,
	};

	mDescriptorSetLayouts.resize(2);

	VK_CHECK("create 0 dsl", vkCreateDescriptorSetLayout(mDevice, &dsl_0_ci, nullptr, &mDescriptorSetLayouts[0]));
	VK_CHECK("create 1 dsl", vkCreateDescriptorSetLayout(mDevice, &dsl_1_ci, nullptr, &mDescriptorSetLayouts[1]));

	const VkPipelineLayoutCreateInfo lyt_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = static_cast<uint32_t>(std::size(mDescriptorSetLayouts)),
		.pSetLayouts = mDescriptorSetLayouts.data(),
	};

	VK_CHECK("create graphics pipeline layout", vkCreatePipelineLayout(vulkan_interface->GetDevice()->GetDevice(), &lyt_ci, nullptr, &mPipelineLayout));

	const VkFormat col_attach_forms[] = {
		VK_FORMAT_R8G8B8A8_UNORM,
	};

	const VkPipelineRenderingCreateInfo rend_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = std::size(col_attach_forms),
		.pColorAttachmentFormats = col_attach_forms,
		.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
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
			.pDepthStencilState = &dss,
			.pColorBlendState = &cbs_ci,
			.pDynamicState = &ds_ci,
			.layout = mPipelineLayout,
		},
	};

	VK_CHECK("create graphics pipeline", vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, std::size(cis), cis, nullptr, &mPipeline));

	vkDestroyShaderModule(mDevice, vert_mod, nullptr);
	vkDestroyShaderModule(mDevice, frag_mod, nullptr);

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(mPipeline), std::string(name).append(" pipeline").c_str());
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_PIPELINE_LAYOUT, reinterpret_cast<uint64_t>(mPipelineLayout), std::string(name).append(" pipeline layout").c_str());

	for (size_t dsl = 0; dsl < mDescriptorSetLayouts.size(); ++dsl)
	{
		Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, reinterpret_cast<uint64_t>(mDescriptorSetLayouts[dsl]), std::string(name).append(" descriptor set layout ").append(std::to_string(dsl)));
	}
#endif // _DEBUG
}

RasterizerPipelineData::~RasterizerPipelineData() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
	{
		for (auto& desc_set_layout : mDescriptorSetLayouts)
			vkDestroyDescriptorSetLayout(mDevice, desc_set_layout, nullptr);

		vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
		vkDestroyPipeline(mDevice, mPipeline, nullptr);
	}
}

VkPipeline RasterizerPipelineData::GetPipeline() const
{
	return mPipeline;
}

VkPipelineLayout RasterizerPipelineData::GetPipelineLayout() const
{
	return mPipelineLayout;
}

const std::vector<VkDescriptorSetLayout>& RasterizerPipelineData::GetDescriptorSetLayouts() const
{
	return mDescriptorSetLayouts;
}

Rasterizer::Rasterizer(const VulkanInterface* vulkan_interface, const std::string& current_path, const std::string& name) :
	mDevice(vulkan_interface->GetDevice()->GetDevice()),
	mSwapchain(vulkan_interface->GetSwapchain()),
	mQueue(vulkan_interface->GetDevice()->GetGraphicsQueue()),
	mAllocator(vulkan_interface->GetAllocator()),
	mQueueFamilyIndices({
			vulkan_interface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex,
			vulkan_interface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex,
			vulkan_interface->GetPhysicalDeviceData()->TransferQueueFamilyIndex,
		}),
		mExtent({
				vulkan_interface->GetSurfaceKHR()->GetSurfaceCapabilities().surfaceCapabilities.currentExtent.width,
				vulkan_interface->GetSurfaceKHR()->GetSurfaceCapabilities().surfaceCapabilities.currentExtent.height,
			})
{
	mMaxFramesInFlight = static_cast<uint8_t>(vulkan_interface->GetSwapchain()->GetImagesCount()) + 2;
	mFrameObjects = std::make_unique<FrameObjects>(vulkan_interface->GetDevice()->GetDevice(), vulkan_interface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex, mMaxFramesInFlight, "rasterizer frame objects");
	mPipelineData = std::make_unique<RasterizerPipelineData>(vulkan_interface, current_path, "rasterizer pipeline data");
	mDepthTexture = std::make_unique<ImageResource>(
		vulkan_interface->GetDevice()->GetDevice(),
		VkExtent3D{
			mExtent.width,
			mExtent.height,
			1,
		},
		VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, vulkan_interface->GetAllocator()->GetAllocator(),
		mQueueFamilyIndices,
		"raster depth texture"
		);

	mAcquireSignalSemaphores.resize(mMaxFramesInFlight, VK_NULL_HANDLE);
	mPresentWaitSemaphores.resize(mMaxFramesInFlight, VK_NULL_HANDLE);

	const VkSemaphoreCreateInfo bin_sem_ci = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	for (uint8_t fr = 0; fr < mMaxFramesInFlight; ++fr)
	{
		VK_CHECK("create acq sig semaphore", vkCreateSemaphore(mDevice, &bin_sem_ci, nullptr, mAcquireSignalSemaphores.data() + fr));
		VK_CHECK("create present wait semaphore", vkCreateSemaphore(mDevice, &bin_sem_ci, nullptr, mPresentWaitSemaphores.data() + fr));

#ifdef _DEBUG
		Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(mAcquireSignalSemaphores[fr]), std::string("raster acq sig sem ").append(std::to_string(fr).c_str()));
		Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(mPresentWaitSemaphores[fr]), std::string("raster prsnt wait sem ").append(std::to_string(fr).c_str()));
#endif // _DEBUG
	}

	InitializeResources(vulkan_interface->GetTransferObjects()->GetCommandBuffer(), vulkan_interface->GetTransferObjects()->GetQueue());
}

void Rasterizer::InitializeResources(const VkCommandBuffer cmd_buff, const VkQueue queue)
{
	const VkCommandBufferBeginInfo cmd_buff_bi = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};

	VK_CHECK("begin xfer cmd buff", vkBeginCommandBuffer(cmd_buff, &cmd_buff_bi));

	Utils_ChangeImageLayout(
		cmd_buff,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		mDepthTexture->GetImage()
	);

	VK_CHECK("end xfer cmd buff", vkEndCommandBuffer(cmd_buff));

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = cmd_buff,
		}
	};

	const VkSubmitInfo2 submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = std::size(cmd_buff_infos),
			.pCommandBufferInfos = cmd_buff_infos,
		},
	};

	VK_CHECK("submit xfer cmd buff", vkQueueSubmit2KHR(queue, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));
	VK_CHECK("wait for device", vkDeviceWaitIdle(mDevice));
}

void Rasterizer::Render(const Scene* scene, ImGUIState* imgui_state)
{
	VkDevice device = mDevice;
	VkCommandBuffer cmd_buff = mFrameObjects->GetCommandBuffer();
	VkSemaphore frame_sem = mFrameObjects->GetSemaphore();
	uint64_t& frame_sem_value = mFrameObjects->GetFrameSemValue();
	uint8_t frame_in_flight = mFrameObjects->GetFrameInFlight();

	const VkSemaphoreWaitInfo wait_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		.semaphoreCount = 1,
		.pSemaphores = &frame_sem,
		.pValues = &frame_sem_value,
	};

	VK_CHECK("wait acq img", vkWaitSemaphoresKHR(device, &wait_info, UINT64_MAX));

	const VkAcquireNextImageInfoKHR acq_info = {
		.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
		.swapchain = mSwapchain->GetSwapchain(),
		.timeout = UINT64_MAX,
		.semaphore = mAcquireSignalSemaphores[frame_in_flight],
		.deviceMask = 0x1,
	};

	uint32_t img_idx = 0;
	VK_CHECK("acq img idx", vkAcquireNextImage2KHR(device, &acq_info, &img_idx));

	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	VK_CHECK("begin rasterizer cmd buff", vkBeginCommandBuffer(cmd_buff, &begin_info));

	Utils_ChangeImageLayout(cmd_buff,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		VK_IMAGE_ASPECT_COLOR_BIT,
		mSwapchain->GetImages()[img_idx]
	);

	VkRenderingAttachmentInfo col_attachs[] = {
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = mSwapchain->GetImageViews()[img_idx],
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

	VkRenderingAttachmentInfo depth_attachment_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = mDepthTexture->GetDescriptorInfo().imageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = {
			.depthStencil = {
				.depth = 1.f,
			},
		},
	};

	const VkRenderingInfo rendering_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = {
			.extent = mExtent,
		},
		.layerCount = 1,
		.colorAttachmentCount = std::size(col_attachs),
		.pColorAttachments = col_attachs,
		.pDepthAttachment = &depth_attachment_info,
	};

	vkCmdBeginRenderingKHR(cmd_buff, &rendering_info);

	const VkViewport viewports[] = {
		{
			.width = 1920.f,//static_cast<float>(mExtent.width),
			.height = 1080.f,//static_cast<float>(mExtent.height),
			.minDepth = 0.f,
			.maxDepth = 1.f,
		},
	};

	const VkRect2D scissors[] = {
		{
			.extent = mExtent,
		},
	};

	vkCmdSetScissor(cmd_buff, 0, std::size(scissors), scissors);
	vkCmdSetViewport(cmd_buff, 0, std::size(viewports), viewports);

	vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineData->GetPipeline());

	scene->Render(device, cmd_buff, mPipelineData->GetPipelineLayout(), imgui_state->GetSelectedCameraIndex());
	imgui_state->Render(cmd_buff);

	vkCmdEndRenderingKHR(cmd_buff);

	Utils_ChangeImageLayout(cmd_buff,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		VK_IMAGE_ASPECT_COLOR_BIT,
		mSwapchain->GetImages()[img_idx]
	);

	VK_CHECK("end rasterizer cmd buff", vkEndCommandBuffer(cmd_buff));

	const VkSemaphoreSubmitInfo wait_sem_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = mAcquireSignalSemaphores[frame_in_flight],
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
			.semaphore = mPresentWaitSemaphores[frame_in_flight],
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

	VK_CHECK("submit rasterizer render commands", vkQueueSubmit2KHR(mQueue, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));

	VkSwapchainKHR swapchain = mSwapchain->GetSwapchain();
	const VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = mPresentWaitSemaphores.data() + frame_in_flight,
		.swapchainCount = 1,
		.pSwapchains = &swapchain,
		.pImageIndices = &img_idx,
	};

	VK_CHECK("q present", vkQueuePresentKHR(mQueue, &present_info));

	// vkQueueWaitIdle is required for the compute queue to fly. 
	// Else cmpt queue with the gfx queue, WIERD!!!
	// Need to check
	VK_CHECK("gfx q wait idle", vkQueueWaitIdle(mQueue));

	mFrameObjects->NextFrame();
}

void Rasterizer::UpdateSwapchain(Swapchain* swapchain)
{
	mSwapchain = swapchain;
}

void Rasterizer::UpdateExtent(const VkExtent2D& extent)
{
	mExtent = extent;
}

void Rasterizer::RecreateDepthTexture()
{
	mDepthTexture = std::make_unique<ImageResource>(
		mDevice,
		VkExtent3D{
			mExtent.width,
			mExtent.height,
			1,
		},
		VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, mAllocator->GetAllocator(),
		mQueueFamilyIndices,
		"raster depth texture"
		);
}

const std::vector<VkDescriptorSetLayout>& Rasterizer::GetDescriptorSetLayouts() const
{
	return mPipelineData->GetDescriptorSetLayouts();
}

Rasterizer::~Rasterizer() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
	{
		for (uint8_t fr = 0; fr < mMaxFramesInFlight; ++fr)
		{
			vkDestroySemaphore(mDevice, mAcquireSignalSemaphores[fr], nullptr);
			vkDestroySemaphore(mDevice, mPresentWaitSemaphores[fr], nullptr);
		}
	}
	vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
}
