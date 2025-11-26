#include "raytrace.hpp"
#include "frame_objects.hpp"
#include "utils.hpp"

struct RaytracePipelineData
{
	struct PushConstants
	{
		uint32_t CurrentSample = 1;
	};

	VkPipeline Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> ShaderGroups;

	VkDevice Device = VK_NULL_HANDLE;
};

RaytracePipelineData RaytracePipelineData_Create(const VulkanInterface* const vulkan_interface, const std::string& current_path)
{
	RaytracePipelineData rpd = {
		.Device = vulkan_interface->DeviceData.Device,
	};

	rpd.DescriptorSetLayouts.resize(1);

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
	VK_CHECK("create shader module", vkCreateShaderModule(vulkan_interface->DeviceData.Device, &rg_mod_ci, nullptr, &rg_mod));

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
	VK_CHECK("create shader module", vkCreateShaderModule(vulkan_interface->DeviceData.Device, &ms_mod_ci, nullptr, &ms_mod));

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
	VK_CHECK("create shader module", vkCreateShaderModule(vulkan_interface->DeviceData.Device, &ch_mod_ci, nullptr, &ch_mod));

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

	VK_CHECK("create desc set layout", vkCreateDescriptorSetLayout(vulkan_interface->DeviceData.Device, &dsl_ci, nullptr, &rpd.DescriptorSetLayouts[0]));

	const VkPushConstantRange pc_ranges[] = {
		{
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			.size = sizeof(RaytracePipelineData::PushConstants),
		},
	};

	const VkPipelineLayoutCreateInfo pl_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = static_cast<uint32_t>(rpd.DescriptorSetLayouts.size()),
		.pSetLayouts = rpd.DescriptorSetLayouts.data(),
		.pushConstantRangeCount = std::size(pc_ranges),
		.pPushConstantRanges = pc_ranges,
	};

	VK_CHECK("create pipeline layout", vkCreatePipelineLayout(vulkan_interface->DeviceData.Device, &pl_ci, nullptr, &rpd.PipelineLayout));

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

	rpd.ShaderGroups = {
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
			.groupCount = static_cast<uint32_t>(std::size(rpd.ShaderGroups)),
			.pGroups = rpd.ShaderGroups.data(),
			.maxPipelineRayRecursionDepth = 1,
			.layout = rpd.PipelineLayout,
		},
	};

	VK_CHECK("create rt pipeline", vkCreateRayTracingPipelinesKHR(vulkan_interface->DeviceData.Device, VK_NULL_HANDLE, VK_NULL_HANDLE, std::size(create_infos), create_infos, nullptr, &rpd.Pipeline));

	//rt_pipeline.rg_sbt = vk_buffer::create(
	//	device, allocator, aligned_handle_size,
	//	VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	//	VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
	//	VMA_MEMORY_USAGE_AUTO_PREFER_HOST, "rb sbt");

	vkDestroyShaderModule(vulkan_interface->DeviceData.Device, rg_mod, nullptr);
	vkDestroyShaderModule(vulkan_interface->DeviceData.Device, ch_mod, nullptr);
	vkDestroyShaderModule(vulkan_interface->DeviceData.Device, ms_mod, nullptr);

	return rpd;
}

void RaytracePipelineData_Destroy(RaytracePipelineData* rpd)
{
	if (rpd->Device != VK_NULL_HANDLE)
	{
		for (auto& desc_set_layout : rpd->DescriptorSetLayouts)
			vkDestroyDescriptorSetLayout(rpd->Device, desc_set_layout, nullptr);

		vkDestroyPipelineLayout(rpd->Device, rpd->PipelineLayout, nullptr);
		vkDestroyPipeline(rpd->Device, rpd->Pipeline, nullptr);
	}
}

struct Raytrace
{
	ImageResource FinalRenderTarget = {};
	ImageResource AccumRenderTarget = {};
	BufferResource RandomStates = {};
	BufferResource RaygenSBT = {};

	FrameObjects* FrameObjects = nullptr;
	std::vector<VkDescriptorSet> DescriptorSets;
	VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR RayTracingProperties = {};

	RaytracePipelineData PipelineData = {};

	VkExtent2D Extent = {};
	VkQueue ComputeQueue = VK_NULL_HANDLE;
	VkQueue TransferQueue = VK_NULL_HANDLE;
	VkCommandBuffer TransferCommandBuffer = VK_NULL_HANDLE;
	std::vector<uint32_t> QueueFamilyIndices;
	VmaAllocator Allocator = nullptr;
	VkDevice Device = VK_NULL_HANDLE;

	uint8_t MaxFramesInFlight = 0;
	uint8_t FrameInFlight = 0;
	bool StopRendering = false;
};

void Raytrace_InitializeResources(Raytrace* r)
{
	const VkCommandBufferBeginInfo cmd_buff_bi = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};

	VK_CHECK("begin xfer cmd buff", vkBeginCommandBuffer(r->TransferCommandBuffer, &cmd_buff_bi));

	Utils_ChangeImageLayout(r->TransferCommandBuffer,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		r->AccumRenderTarget.Image
	);

	BufferResource staging_buffer = BufferResource_Create(
		r->Device, r->Allocator,
		r->RandomStates.AllocationInfo.allocationInfo.size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST, "staging rand states buffer");

	for (uint32_t st = 0; st < 4 * r->Extent.width * r->Extent.height;)
	{
		uint32_t rand_val = rand();
		while (rand_val < 128)
			rand_val = rand();

		(reinterpret_cast<uint32_t*>(staging_buffer.AllocationInfo.allocationInfo.pMappedData))[st++] = rand_val;
	}

	const VkBufferCopy2 regions[] = {
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
			.size = r->RandomStates.AllocationInfo.allocationInfo.size,
		},
	};

	const VkCopyBufferInfo2 copy_buff_info = {
		.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
		.srcBuffer = staging_buffer.DescriptorInfo.buffer,
		.dstBuffer = r->RandomStates.DescriptorInfo.buffer,
		.regionCount = std::size(regions),
		.pRegions = regions,
	};

	vkCmdCopyBuffer2(r->TransferCommandBuffer, &copy_buff_info);

	VK_CHECK("end xfer cmd buff", vkEndCommandBuffer(r->TransferCommandBuffer));

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = r->TransferCommandBuffer,
		}
	};

	const VkSubmitInfo2 submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = std::size(cmd_buff_infos),
			.pCommandBufferInfos = cmd_buff_infos,
		},
	};

	VK_CHECK("submit xfer cmd buff", vkQueueSubmit2(r->TransferQueue, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));
	VK_CHECK("wait for device", vkDeviceWaitIdle(r->Device));

	BufferResource_Destroy(staging_buffer);

	const uint32_t aligned_handle_size = static_cast<uint32_t>(ALIGNED_SIZE(r->RayTracingProperties.shaderGroupHandleSize, r->RayTracingProperties.shaderGroupHandleAlignment));
	const uint32_t sbt_size = aligned_handle_size * static_cast<uint32_t>(std::size(r->PipelineData.ShaderGroups));

	std::vector<uint8_t> shader_handle_storage(sbt_size);
	VK_CHECK("get rt shader handles", vkGetRayTracingShaderGroupHandlesKHR(r->Device, r->PipelineData.Pipeline, 0, static_cast<uint32_t>(std::size(r->PipelineData.ShaderGroups)), sbt_size, shader_handle_storage.data()));

	memcpy(r->RaygenSBT.AllocationInfo.allocationInfo.pMappedData, shader_handle_storage.data(), sbt_size);
}

Raytrace* Raytrace_Create(const VulkanInterface* const vulkan_interface, const VkExtent2D& extent, const std::string& current_path)
{
	Raytrace* r = reinterpret_cast<Raytrace*>(std::calloc(1, sizeof(Raytrace)));

	r->Device = vulkan_interface->DeviceData.Device;
	r->Allocator = vulkan_interface->Allocator;
	r->RayTracingProperties = vulkan_interface->PhysicalDeviceData.RayTracingProperties;
	r->FinalRenderTarget = vulkan_interface->FinalRenderTarget;
	r->AccumRenderTarget = ImageResource_Create(
		r->Device, { extent.width, extent.height, 1 }, VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, vulkan_interface->Allocator, 0,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		{ vulkan_interface->PhysicalDeviceData.GraphicsQueueFamilyIndex, vulkan_interface->PhysicalDeviceData.ComputeQueueFamilyIndex, vulkan_interface->PhysicalDeviceData.TransferQueueFamilyIndex },
		"accum render target");
	r->RandomStates = BufferResource_Create(
		r->Device, vulkan_interface->Allocator, extent.width * extent.height * 4 * sizeof(uint32_t),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, "rand states");
	r->MaxFramesInFlight = static_cast<uint8_t>(vulkan_interface->SwapchainData.ImagesCount);
	r->Extent = extent,
	r->ComputeQueue = vulkan_interface->DeviceData.ComputeQueue;
	r->TransferQueue = vulkan_interface->TransferObjects.Queue;
	r->TransferCommandBuffer = vulkan_interface->TransferObjects.CommandBuffer;
	r->FrameObjects = FrameObjects_Create(r->Device, vulkan_interface->PhysicalDeviceData.ComputeQueueFamilyIndex, r->MaxFramesInFlight);
	r->QueueFamilyIndices = {
		vulkan_interface->PhysicalDeviceData.GraphicsQueueFamilyIndex, vulkan_interface->PhysicalDeviceData.ComputeQueueFamilyIndex,
		vulkan_interface->PhysicalDeviceData.TransferQueueFamilyIndex
	};

	r->PipelineData = RaytracePipelineData_Create(vulkan_interface, current_path);

	r->DescriptorSets.resize(r->MaxFramesInFlight);

	const uint32_t aligned_handle_size = static_cast<uint32_t>(ALIGNED_SIZE(r->RayTracingProperties.shaderGroupHandleSize, r->RayTracingProperties.shaderGroupHandleAlignment));
	r->RaygenSBT = BufferResource_Create(
		r->Device, r->Allocator, aligned_handle_size,
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
		.maxSets = r->MaxFramesInFlight,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes,
	};

	VK_CHECK("create dsp", vkCreateDescriptorPool(r->Device, &dp_ci, nullptr, &r->DescriptorPool));

	const VkDescriptorSetAllocateInfo ds_ai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = r->DescriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = r->PipelineData.DescriptorSetLayouts.data(),
	};

	for (uint8_t fr = 0; fr < r->MaxFramesInFlight; ++fr)
	{
		VK_CHECK("allocate raytrace desc sets", vkAllocateDescriptorSets(r->Device, &ds_ai, r->DescriptorSets.data() + fr));
	}

	Raytrace_InitializeResources(r);

	return r;
}

void Raytrace_Destroy(Raytrace* r)
{
	if (r->Device != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(r->Device, r->DescriptorPool, nullptr);

	BufferResource_Destroy(r->RaygenSBT);
	BufferResource_Destroy(r->RandomStates);
	ImageResource_Destroy(r->AccumRenderTarget);
	FrameObjects_Destroy(r->FrameObjects);
	RaytracePipelineData_Destroy(&r->PipelineData);

	free(r);
}

void Raytrace_RecreateRenderResources(Raytrace* r, const VkExtent2D& extent)
{
	ImageResource_Destroy(r->AccumRenderTarget);
	BufferResource_Destroy(r->RandomStates);

	r->Extent = extent;
	r->AccumRenderTarget = ImageResource_Create(r->Device, { r->Extent.width, r->Extent.height, 1 }, VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, r->Allocator,
		0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, r->QueueFamilyIndices, "accum render target");
	r->RandomStates = BufferResource_Create(r->Device, r->Allocator, r->Extent.width * r->Extent.height * 4 * sizeof(uint32_t),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, "random states");

	Raytrace_InitializeResources(r);
}

void Raytrace_Render(Raytrace* r, bool* is_rendering, const uint32_t max_samples)
{
	VkDevice device = r->Device;
	VkCommandBuffer cmd_buff = FrameObjects_GetCommandBuffer(r->FrameObjects);
	VkSemaphore frame_sem = FrameObjects_GetSemaphore(r->FrameObjects);
	uint64_t& frame_sem_value = FrameObjects_GetFrameSemValue(r->FrameObjects);
	uint8_t frame_in_flight = FrameObjects_GetFrameInFlight(r->FrameObjects);

	uint32_t s = 1;

	do {

		const VkSemaphoreWaitInfo wait_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &frame_sem,
			.pValues = &frame_sem_value,
		};

		VK_CHECK("wait acq img", vkWaitSemaphores(device, &wait_info, UINT64_MAX));

		if (r->StopRendering) break;

		const VkCommandBufferBeginInfo rt_begin_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};
		VK_CHECK("begin rt cmd_buff", vkBeginCommandBuffer(cmd_buff, &rt_begin_info));

		Utils_InsertMemoryBarrier(cmd_buff,
			VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR | VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT
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

			vkCmdClearColorImage(cmd_buff, r->AccumRenderTarget.Image, VK_IMAGE_LAYOUT_GENERAL, &clear_color, std::size(ranges), ranges);
		}

		vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, r->PipelineData.Pipeline);

		const VkWriteDescriptorSet rt_desc_writes[] = {
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = r->DescriptorSets[frame_in_flight],
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.pImageInfo = &r->AccumRenderTarget.DescriptorInfo,
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = r->DescriptorSets[frame_in_flight],
				.dstBinding = 1,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.pImageInfo = &r->FinalRenderTarget.DescriptorInfo,
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = r->DescriptorSets[frame_in_flight],
				.dstBinding = 2,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &r->RandomStates.DescriptorInfo,
			},
		};

		vkUpdateDescriptorSets(device, std::size(rt_desc_writes), rt_desc_writes, 0, nullptr);

		const VkBindDescriptorSetsInfo rt_ds_bi = {
			.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			.layout = r->PipelineData.PipelineLayout,
			.descriptorSetCount = 1,
			.pDescriptorSets = r->DescriptorSets.data() + frame_in_flight,
		};

		vkCmdBindDescriptorSets2(cmd_buff, &rt_ds_bi);

		const RaytracePipelineData::PushConstants rt_pc = {
			.CurrentSample = s,
		};

		const VkPushConstantsInfo rt_pc_info = {
			.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
			.layout = r->PipelineData.PipelineLayout,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			.size = sizeof(RaytracePipelineData::PushConstants),
			.pValues = &rt_pc,
		};

		vkCmdPushConstants2(cmd_buff, &rt_pc_info);

		const VkBufferDeviceAddressInfo rg_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = r->RaygenSBT.DescriptorInfo.buffer,
		};

		const VkStridedDeviceAddressRegionKHR rg_sbt = {
			.deviceAddress = vkGetBufferDeviceAddress(device, &rg_info),
			.stride = r->RayTracingProperties.shaderGroupHandleSize,
			.size = r->RayTracingProperties.shaderGroupHandleSize,
		};

		const VkStridedDeviceAddressRegionKHR ms_sbt = {};
		const VkStridedDeviceAddressRegionKHR ch_sbt = {};
		const VkStridedDeviceAddressRegionKHR cl_sbt = {};

		vkCmdTraceRaysKHR(cmd_buff, &rg_sbt, &ms_sbt, &ch_sbt, &cl_sbt, r->Extent.width, r->Extent.height, 1);

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

		VK_CHECK("submit rt commamds", vkQueueSubmit2(r->ComputeQueue, std::size(rt_submit_infos), rt_submit_infos, VK_NULL_HANDLE));

		FrameObjects_NextFrame(r->FrameObjects);
	} while (++s <= max_samples);

	VK_CHECK("raytrace queue wait idle", vkQueueWaitIdle(r->ComputeQueue));

	*is_rendering = false;
	r->StopRendering = false;
}

void Raytrace_UpdateFinalRenderTarget(Raytrace* r, const ImageResource FinalRenderTarget)
{
	r->FinalRenderTarget = FinalRenderTarget;
}

void Raytrace_StopRendering(Raytrace* r)
{
	r->StopRendering = true;
}
