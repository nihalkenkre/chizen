#include "raytrace.hpp"
#include "frame_objects.hpp"
#include "utils.hpp"
#include "vulkan_interface.hpp"
#include "resources.hpp"
#include "vulkan_objects.hpp"

class RaytracePipelineData
{
public:
	RaytracePipelineData() = delete;
	RaytracePipelineData(const VulkanInterface* const vulkan_interface, const std::string& current_path, const std::string& name);

	RaytracePipelineData(const RaytracePipelineData& other) = delete;
	RaytracePipelineData& operator=(const RaytracePipelineData& other) = delete;

	~RaytracePipelineData() noexcept;

	struct PushConstants
	{
		uint32_t CurrentSample = 1;
	};

	VkPipeline GetPipeline() const;
	VkPipelineLayout GetPipelineLayout() const;
	std::vector<VkDescriptorSetLayout> GetDescriptorSetLayouts() const;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> GetShaderGroups() const;

private:
	VkPipeline mPipeline = VK_NULL_HANDLE;
	VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> mShaderGroups;

	VkDevice mDevice = VK_NULL_HANDLE;
};

RaytracePipelineData::RaytracePipelineData(const VulkanInterface* const vulkan_interface, const std::string& current_path, const std::string& name)
{
	mDevice = vulkan_interface->GetDevice()->GetDevice();

	mDescriptorSetLayouts.resize(1);

	Slang::ComPtr<slang::IGlobalSession> slang_global_session;
	SLANG_CHECK("create global session", slang::createGlobalSession(slang_global_session.writeRef()));

	const slang::TargetDesc target_descs[] = {
		{
			.format = SLANG_SPIRV,
			.profile = slang_global_session->findProfile("spirv_1_1"),
		}
	};

#ifdef _DEBUG
	slang::CompilerOptionEntry compiler_options[] = {
		{
			.name = slang::CompilerOptionName::DebugInformation,
			.value = {
				.kind = slang::CompilerOptionValueKind::Int,
				.intValue0 = SLANG_DEBUG_INFO_LEVEL_MAXIMAL,
			},
		}
	};
#endif	// _DEBUG

	slang::SessionDesc compile_session_desc = {
		.targets = target_descs,
		.targetCount = std::size(target_descs),
#ifdef _DEBUG
		.compilerOptionEntries = compiler_options,
		.compilerOptionEntryCount = std::size(compiler_options),
#endif	// _DEBUG
	};

	Slang::ComPtr<slang::ISession> compile_session;
	SLANG_CHECK("create compile session", slang_global_session->createSession(compile_session_desc, compile_session.writeRef()));

	const std::string slang_shader_path = std::string(current_path).append("/shaders/slang/raytrace.slang");

	Slang::ComPtr<slang::IBlob> diagnostic_blob;
	slang::IModule* slang_module = compile_session->loadModule(slang_shader_path.c_str(), diagnostic_blob.writeRef());

	if (diagnostic_blob != nullptr)
	{
		std::println("{}", reinterpret_cast<const char*>(diagnostic_blob->getBufferPointer()));
	}

	Slang::ComPtr<slang::IEntryPoint> rg_entry_point;
	slang_module->findEntryPointByName("raygen", rg_entry_point.writeRef());

	std::array<slang::IComponentType*, 2> rg_component_types = {
		slang_module, rg_entry_point
	};

	Slang::ComPtr<slang::IComponentType> rg_composed_program;
	diagnostic_blob.setNull();
	SLANG_CHECK("create program", compile_session->createCompositeComponentType(rg_component_types.data(), rg_component_types.size(), rg_composed_program.writeRef(), diagnostic_blob.writeRef()));

	Slang::ComPtr<slang::IComponentType> rg_linked_program;
	diagnostic_blob.setNull();
	SLANG_CHECK("link program", rg_composed_program->link(rg_linked_program.writeRef(), diagnostic_blob.writeRef()));

	Slang::ComPtr<slang::IBlob> rg_spirv_code;
	diagnostic_blob.setNull();
	SLANG_CHECK("get spirv code", rg_composed_program->getEntryPointCode(0, 0, rg_spirv_code.writeRef(), diagnostic_blob.writeRef()));

	const VkShaderModuleCreateInfo rg_mod_ci = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = rg_spirv_code->getBufferSize(),
		.pCode = reinterpret_cast<const uint32_t*>(rg_spirv_code->getBufferPointer()),
	};

	VkShaderModule rg_mod = VK_NULL_HANDLE;
	VK_CHECK("create shader module", vkCreateShaderModule(vulkan_interface->GetDevice()->GetDevice(), &rg_mod_ci, nullptr, &rg_mod));

	Slang::ComPtr<slang::IEntryPoint> ms_entry_point;
	slang_module->findEntryPointByName("miss", ms_entry_point.writeRef());

	std::array<slang::IComponentType*, 2> ms_component_types = {
		slang_module, ms_entry_point
	};

	Slang::ComPtr<slang::IComponentType> ms_composed_program;
	diagnostic_blob.setNull();
	SLANG_CHECK("create program", compile_session->createCompositeComponentType(ms_component_types.data(), ms_component_types.size(), ms_composed_program.writeRef(), diagnostic_blob.writeRef()));

	Slang::ComPtr<slang::IComponentType> ms_linked_program;
	diagnostic_blob.setNull();
	SLANG_CHECK("link program", ms_composed_program->link(ms_linked_program.writeRef(), diagnostic_blob.writeRef()));

	Slang::ComPtr<slang::IBlob> ms_spirv_code;
	diagnostic_blob.setNull();
	SLANG_CHECK("get spirv code", ms_composed_program->getEntryPointCode(0, 0, ms_spirv_code.writeRef(), diagnostic_blob.writeRef()));

	const VkShaderModuleCreateInfo ms_mod_ci = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = ms_spirv_code->getBufferSize(),
		.pCode = reinterpret_cast<const uint32_t*>(ms_spirv_code->getBufferPointer()),
	};

	VkShaderModule ms_mod = VK_NULL_HANDLE;
	VK_CHECK("create shader module", vkCreateShaderModule(vulkan_interface->GetDevice()->GetDevice(), &ms_mod_ci, nullptr, &ms_mod));

	Slang::ComPtr<slang::IEntryPoint> ch_entry_point;
	slang_module->findEntryPointByName("closesthit", ch_entry_point.writeRef());

	std::array<slang::IComponentType*, 2> ch_component_types = {
		slang_module, ch_entry_point
	};

	Slang::ComPtr<slang::IComponentType> ch_composed_program;
	diagnostic_blob.setNull();
	SLANG_CHECK("create program", compile_session->createCompositeComponentType(ch_component_types.data(), ch_component_types.size(), ch_composed_program.writeRef(), diagnostic_blob.writeRef()));

	Slang::ComPtr<slang::IComponentType> ch_linked_program;
	diagnostic_blob.setNull();
	SLANG_CHECK("link program", ch_composed_program->link(ch_linked_program.writeRef(), diagnostic_blob.writeRef()));

	Slang::ComPtr<slang::IBlob> ch_spirv_code;
	diagnostic_blob.setNull();
	SLANG_CHECK("get spirv code", ch_composed_program->getEntryPointCode(0, 0, ch_spirv_code.writeRef(), diagnostic_blob.writeRef()));

	const VkShaderModuleCreateInfo ch_mod_ci = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = ch_spirv_code->getBufferSize(),
		.pCode = reinterpret_cast<const uint32_t*>(ch_spirv_code->getBufferPointer()),
	};

	VkShaderModule ch_mod = VK_NULL_HANDLE;
	VK_CHECK("create shader module", vkCreateShaderModule(vulkan_interface->GetDevice()->GetDevice(), &ch_mod_ci, nullptr, &ch_mod));

	const VkDescriptorSetLayoutBinding bindings[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		},
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		},
		{
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		}
	};

	const VkDescriptorSetLayoutCreateInfo dsl_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = std::size(bindings),
		.pBindings = bindings,
	};

	VK_CHECK("create desc set layout", vkCreateDescriptorSetLayout(vulkan_interface->GetDevice()->GetDevice(), &dsl_ci, nullptr, &mDescriptorSetLayouts[0]));

	const VkPushConstantRange pc_ranges[] = {
		{
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			.size = sizeof(RaytracePipelineData::PushConstants),
		},
	};

	const VkPipelineLayoutCreateInfo pl_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = static_cast<uint32_t>(mDescriptorSetLayouts.size()),
		.pSetLayouts = mDescriptorSetLayouts.data(),
		.pushConstantRangeCount = std::size(pc_ranges),
		.pPushConstantRanges = pc_ranges,
	};

	VK_CHECK("create pipeline layout", vkCreatePipelineLayout(vulkan_interface->GetDevice()->GetDevice(), &pl_ci, nullptr, &mPipelineLayout));

	const VkPipelineShaderStageCreateInfo stage_cis[] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			.module = rg_mod,
			.pName = "main",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_MISS_BIT_KHR,
			.module = ms_mod,
			.pName = "main",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
			.module = ch_mod,
			.pName = "main",
		},
	};

	mShaderGroups = {
		{
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
			.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
			.generalShader = 0,
			.closestHitShader = VK_SHADER_UNUSED_KHR,
			.anyHitShader = VK_SHADER_UNUSED_KHR,
			.intersectionShader = VK_SHADER_UNUSED_KHR,
		},
		{
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
			.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
			.generalShader = 1,
			.closestHitShader = VK_SHADER_UNUSED_KHR,
			.anyHitShader = VK_SHADER_UNUSED_KHR,
			.intersectionShader = VK_SHADER_UNUSED_KHR,
		},
		{
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
			.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
			.generalShader = VK_SHADER_UNUSED_KHR,
			.closestHitShader = 2,
			.anyHitShader = VK_SHADER_UNUSED_KHR,
			.intersectionShader = VK_SHADER_UNUSED_KHR,
		},
	};

	const VkRayTracingPipelineCreateInfoKHR create_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
			.stageCount = std::size(stage_cis),
			.pStages = stage_cis,
			.groupCount = static_cast<uint32_t>(std::size(mShaderGroups)),
			.pGroups = mShaderGroups.data(),
			.maxPipelineRayRecursionDepth = 1,
			.layout = mPipelineLayout,
		},
	};

	VK_CHECK("create rt pipeline", vkCreateRayTracingPipelinesKHR(vulkan_interface->GetDevice()->GetDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE, std::size(create_infos), create_infos, nullptr, &mPipeline));

	vkDestroyShaderModule(vulkan_interface->GetDevice()->GetDevice(), rg_mod, nullptr);
	vkDestroyShaderModule(vulkan_interface->GetDevice()->GetDevice(), ch_mod, nullptr);
	vkDestroyShaderModule(vulkan_interface->GetDevice()->GetDevice(), ms_mod, nullptr);

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(mPipeline), std::string(name).append(" pipeline").c_str());
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_PIPELINE_LAYOUT, reinterpret_cast<uint64_t>(mPipelineLayout), std::string(name).append(" pipeline layout").c_str());

	for (size_t dsl = 0; dsl < mDescriptorSetLayouts.size(); ++dsl)
	{
		Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, reinterpret_cast<uint64_t>(mDescriptorSetLayouts[dsl]), std::string(name).append(" descriptor set layout ").append(std::to_string(dsl)).c_str());
	}

#endif	// _DEBUG
}

RaytracePipelineData::~RaytracePipelineData() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
	{
		for (auto& desc_set_layout : mDescriptorSetLayouts)
			vkDestroyDescriptorSetLayout(mDevice, desc_set_layout, nullptr);

		vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
		vkDestroyPipeline(mDevice, mPipeline, nullptr);
	}
}

VkPipeline RaytracePipelineData::GetPipeline() const
{
	return mPipeline;
}

VkPipelineLayout RaytracePipelineData::GetPipelineLayout() const
{
	return mPipelineLayout;
}

std::vector<VkDescriptorSetLayout> RaytracePipelineData::GetDescriptorSetLayouts() const
{
	return mDescriptorSetLayouts;
}

std::vector<VkRayTracingShaderGroupCreateInfoKHR> RaytracePipelineData::GetShaderGroups() const
{
	return mShaderGroups;
}

void Raytrace::InitializeResources()
{
	const VkCommandBufferBeginInfo cmd_buff_bi = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};

	VK_CHECK("begin xfer cmd buff", vkBeginCommandBuffer(mTransferCommandBuffer, &cmd_buff_bi));

	mAccumRenderTarget->ChangeImageLayout(
		mTransferCommandBuffer,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED
	);

	std::unique_ptr<BufferResource> staging_buffer = std::make_unique<BufferResource>(
		mDevice, mAllocator,
		mRandomStates->GetAllocationInfo2().allocationInfo.size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST, "staging rand states buffer");

	for (uint32_t st = 0; st < 4 * mExtent.width * mExtent.height;)
	{
		uint32_t rand_val = rand();
		while (rand_val < 128)
			rand_val = rand();

		(reinterpret_cast<uint32_t*>(staging_buffer->GetAllocationInfo2().allocationInfo.pMappedData))[st++] = rand_val;
	}

	const VkBufferCopy2 regions[] = {
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
			.size = mRandomStates->GetAllocationInfo2().allocationInfo.size,
		},
	};

	const VkCopyBufferInfo2 copy_buff_info = {
		.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
		.srcBuffer = staging_buffer->GetDescriptorInfo().buffer,
		.dstBuffer = mRandomStates->GetDescriptorInfo().buffer,
		.regionCount = std::size(regions),
		.pRegions = regions,
	};

	vkCmdCopyBuffer2KHR(mTransferCommandBuffer, &copy_buff_info);

	VK_CHECK("end xfer cmd buff", vkEndCommandBuffer(mTransferCommandBuffer));

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = mTransferCommandBuffer,
		}
	};

	const VkSubmitInfo2 submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = std::size(cmd_buff_infos),
			.pCommandBufferInfos = cmd_buff_infos,
		},
	};

	VK_CHECK("submit xfer cmd buff", vkQueueSubmit2KHR(mTransferQueue, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));
	VK_CHECK("wait for device", vkDeviceWaitIdle(mDevice));

	const uint32_t aligned_handle_size = static_cast<uint32_t>(ALIGNED_SIZE(mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment));
	const uint32_t sbt_size = aligned_handle_size * static_cast<uint32_t>(std::size(mPipelineData->GetShaderGroups()));

	std::vector<uint8_t> shader_handle_storage(sbt_size);
	VK_CHECK("get rt shader handles", vkGetRayTracingShaderGroupHandlesKHR(mDevice, mPipelineData->GetPipeline(), 0, static_cast<uint32_t>(std::size(mPipelineData->GetShaderGroups())), sbt_size, shader_handle_storage.data()));

	memcpy(mRaygenSBT->GetAllocationInfo2().allocationInfo.pMappedData, shader_handle_storage.data(), sbt_size);
}

Raytrace::Raytrace(const VulkanInterface* const vulkan_interface, ImageResource* final_render_target, const VkExtent3D& extent, const std::string& current_path, const std::string& name)
{
	mDevice = vulkan_interface->GetDevice()->GetDevice();
	mAllocator = vulkan_interface->GetAllocator()->GetAllocator();
	mRayTracingProperties = vulkan_interface->GetPhysicalDeviceData()->RayTracingProperties;
	mFinalRenderTarget = final_render_target;
	mQueueFamilyIndices = {
		vulkan_interface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex,
		vulkan_interface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex,
		vulkan_interface->GetPhysicalDeviceData()->TransferQueueFamilyIndex
	};
	mAccumRenderTarget = std::make_unique<ImageResource>(
		mDevice, extent, VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, vulkan_interface->GetAllocator()->GetAllocator(), 0,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		mQueueFamilyIndices, "accum render target");
	mRandomStates = std::make_unique<BufferResource>(
		mDevice, vulkan_interface->GetAllocator()->GetAllocator(), extent.width * extent.height * 4 * sizeof(uint32_t),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, "rand states");
	mMaxFramesInFlight = static_cast<uint8_t>(vulkan_interface->GetSwapchain()->GetImagesCount());
	mExtent = extent;
	mComputeQueue = vulkan_interface->GetDevice()->GetComputeQueue();
	mTransferQueue = vulkan_interface->GetTransferObjects()->GetQueue();
	mTransferCommandBuffer = vulkan_interface->GetTransferObjects()->GetCommandBuffer();
	mFrameObjects = std::make_unique<FrameObjects>(mDevice, vulkan_interface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex, mMaxFramesInFlight, "raytrace frame objects");

	mPipelineData = std::make_unique<RaytracePipelineData>(vulkan_interface, current_path, "reytrace pipeline data");

	mDescriptorSets.resize(mMaxFramesInFlight);

	const uint32_t aligned_handle_size = static_cast<uint32_t>(ALIGNED_SIZE(mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment));
	mRaygenSBT = std::make_unique<BufferResource>(
		mDevice, mAllocator, aligned_handle_size,
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST, "rb sbt");

	const VkDescriptorPoolSize pool_sizes[] = {
		{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
		},
	};

	const VkDescriptorPoolCreateInfo dp_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = mMaxFramesInFlight,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes,
	};

	VK_CHECK("create dsp", vkCreateDescriptorPool(mDevice, &dp_ci, nullptr, &mDescriptorPool));

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_DESCRIPTOR_POOL, reinterpret_cast<uint64_t>(mDescriptorPool), std::string(name).append(" descriptor pool").c_str());
#endif

	auto dsls = mPipelineData->GetDescriptorSetLayouts();
	const VkDescriptorSetAllocateInfo ds_ai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = mDescriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = dsls.data(),
	};

	for (uint8_t fr = 0; fr < mMaxFramesInFlight; ++fr)
	{
		VK_CHECK("allocate raytrace desc sets", vkAllocateDescriptorSets(mDevice, &ds_ai, mDescriptorSets.data() + fr));
#ifdef _DEBUG
		Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(mDescriptorSets[fr]), std::string(name).append(" descriptor set ").append(std::to_string(fr).c_str()));
#endif	// _DBEUG
	}

	InitializeResources();
}

Raytrace::~Raytrace()
{
	if (mDevice != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
}

void Raytrace::RecreateRenderResources(const VkExtent2D& extent)
{
	mExtent = { extent.width , extent.height, 1 };
	mAccumRenderTarget = std::make_unique<ImageResource>(mDevice, mExtent, VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, mAllocator,
		0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, mQueueFamilyIndices, "accum render target");
	mRandomStates = std::make_unique<BufferResource>(mDevice, mAllocator, mExtent.width * mExtent.height * 4 * sizeof(uint32_t),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, "random states");

	InitializeResources();
}

void Raytrace::Render(bool* is_raytracing, const uint32_t max_samples)
{
	VkDevice device = mDevice;
	VkCommandBuffer cmd_buff = mFrameObjects->GetCommandBuffer();
	VkSemaphore frame_sem = mFrameObjects->GetSemaphore();
	uint64_t& frame_sem_value = mFrameObjects->GetFrameSemValue();
	uint8_t frame_in_flight = mFrameObjects->GetFrameInFlight();

	uint32_t s = 1;

	do {
		const VkSemaphoreWaitInfo wait_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &frame_sem,
			.pValues = &frame_sem_value,
		};

		VK_CHECK("wait acq img", vkWaitSemaphoresKHR(device, &wait_info, UINT64_MAX));

		// waiting for last submitted buffer to complete before exiting. resources in use.
		if (mStopRendering) break;

		const VkCommandBufferBeginInfo rt_begin_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};
		VK_CHECK("begin rt cmd_buff", vkBeginCommandBuffer(cmd_buff, &rt_begin_info));

		Utils_InsertMemoryBarrier(
			cmd_buff,
			VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR | VK_PIPELINE_STAGE_2_CLEAR_BIT,
			VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT
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

			vkCmdClearColorImage(cmd_buff, mAccumRenderTarget->GetImage(), VK_IMAGE_LAYOUT_GENERAL, &clear_color, std::size(ranges), ranges);
		}

		vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipelineData->GetPipeline());

		VkDescriptorImageInfo accum_target_desc_info = mAccumRenderTarget->GetDescriptorInfo();
		VkDescriptorImageInfo final_render_desc_info = mFinalRenderTarget->GetDescriptorInfo();
		VkDescriptorBufferInfo rand_states_desc_info = mRandomStates->GetDescriptorInfo();

		const VkWriteDescriptorSet rt_desc_writes[] = {
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = mDescriptorSets[frame_in_flight],
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.pImageInfo = &accum_target_desc_info,
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = mDescriptorSets[frame_in_flight],
				.dstBinding = 1,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.pImageInfo = &final_render_desc_info,
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = mDescriptorSets[frame_in_flight],
				.dstBinding = 2,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &rand_states_desc_info,
			},
		};

		vkUpdateDescriptorSets(device, std::size(rt_desc_writes), rt_desc_writes, 0, nullptr);

		const VkBindDescriptorSetsInfo rt_ds_bi = {
			.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			.layout = mPipelineData->GetPipelineLayout(),
			.descriptorSetCount = 1,
			.pDescriptorSets = mDescriptorSets.data() + frame_in_flight,
		};

		vkCmdBindDescriptorSets2KHR(cmd_buff, &rt_ds_bi);

		const RaytracePipelineData::PushConstants rt_pc = {
			.CurrentSample = s,
		};

		const VkPushConstantsInfo rt_pc_info = {
			.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
			.layout = mPipelineData->GetPipelineLayout(),
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			.size = sizeof(RaytracePipelineData::PushConstants),
			.pValues = &rt_pc,
		};

		vkCmdPushConstants2KHR(cmd_buff, &rt_pc_info);

		const VkBufferDeviceAddressInfo rg_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = mRaygenSBT->GetDescriptorInfo().buffer,
		};

		const VkStridedDeviceAddressRegionKHR rg_sbt = {
			.deviceAddress = vkGetBufferDeviceAddressKHR(device, &rg_info),
			.stride = mRayTracingProperties.shaderGroupHandleSize,
			.size = mRayTracingProperties.shaderGroupHandleSize,
		};

		const VkStridedDeviceAddressRegionKHR ms_sbt = {};
		const VkStridedDeviceAddressRegionKHR ch_sbt = {};
		const VkStridedDeviceAddressRegionKHR cl_sbt = {};

		vkCmdTraceRaysKHR(cmd_buff, &rg_sbt, &ms_sbt, &ch_sbt, &cl_sbt, mExtent.width, mExtent.height, 1);

		VK_CHECK("end rt cmd buffer", vkEndCommandBuffer(cmd_buff));

		const VkCommandBufferSubmitInfo rt_cmd_buff_infos[] = {
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
				.commandBuffer = cmd_buff,
			},
		};

		const VkSemaphoreSubmitInfo rt_sig_sem_infos[] = {
			{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = frame_sem,
				.value = ++frame_sem_value,
				.stageMask = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
			},
		};

		const VkSubmitInfo2 rt_submit_infos[] = {
			{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
				.commandBufferInfoCount = std::size(rt_cmd_buff_infos),
				.pCommandBufferInfos = rt_cmd_buff_infos,
				.signalSemaphoreInfoCount = std::size(rt_sig_sem_infos),
				.pSignalSemaphoreInfos = rt_sig_sem_infos,
			},
		};

		VK_CHECK("submit rt commamds", vkQueueSubmit2KHR(mComputeQueue, std::size(rt_submit_infos), rt_submit_infos, VK_NULL_HANDLE));

		mFrameObjects->NextFrame();
	} while (++s <= max_samples);

	// waiting for last submitted buffer to complete before exiting. resources in use.
	VK_CHECK("raytrace queue wait idle", vkQueueWaitIdle(mComputeQueue));

	*is_raytracing = false;
	mStopRendering = false;
}

void Raytrace::UpdateFinalRenderTarget(ImageResource* FinalRenderTarget)
{
	mFinalRenderTarget = FinalRenderTarget;
}

void Raytrace::StopRendering()
{
	mStopRendering = true;
}
