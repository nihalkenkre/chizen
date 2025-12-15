#include "vulkan_raytracer.hpp"
#include "frame_objects.hpp"
#include "utils.hpp"
#include "vulkan_interface.hpp"
#include "resources.hpp"
#include "vulkan_objects.hpp"
#include "events.hpp"
#include "embree_raytracer.hpp"

class RaytracerPipelineData
{
public:
	RaytracerPipelineData() = delete;
	RaytracerPipelineData(const VulkanInterface* const vulkan_interface, const std::string& current_path, const std::string& name);

	RaytracerPipelineData(const RaytracerPipelineData& other) = delete;
	RaytracerPipelineData& operator=(const RaytracerPipelineData& other) = delete;

	~RaytracerPipelineData() noexcept;

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

RaytracerPipelineData::RaytracerPipelineData(const VulkanInterface* const vulkan_interface, const std::string& current_path, const std::string& name)
{
	mDevice = vulkan_interface->GetDevice()->GetDevice();

	mDescriptorSetLayouts.resize(1);

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
	//			},
	//		}
	//#endif	// _DEBUG
	//	};
	//
	//	slang::SessionDesc compile_session_desc = {
	//		.targets = target_descs,
	//		.targetCount = std::size(target_descs),
	//		.compilerOptionEntries = compiler_options,
	//		.compilerOptionEntryCount = std::size(compiler_options),
	//	};
	//
	//	Slang::ComPtr<slang::ISession> compile_session;
	//	SLANG_CHECK("create compile session", slang_global_session->createSession(compile_session_desc, compile_session.writeRef()));
	//
	//	const std::string slang_shader_path = std::string(current_path).append("/shaders/slang/raytrace.slang");
	//
	//	Slang::ComPtr<slang::IBlob> diagnostic_blob;
	//	slang::IModule* slang_module = compile_session->loadModule(slang_shader_path.c_str(), diagnostic_blob.writeRef());
	//
	//	if (diagnostic_blob != nullptr)
	//	{
	//		std::println("{}", reinterpret_cast<const char*>(diagnostic_blob->getBufferPointer()));
	//	}
	//
	//	Slang::ComPtr<slang::IEntryPoint> rg_entry_point;
	//	slang_module->findEntryPointByName("raygen", rg_entry_point.writeRef());
	//
	//	std::array<slang::IComponentType*, 2> rg_component_types = {
	//		slang_module, rg_entry_point
	//	};
	//
	//	Slang::ComPtr<slang::IComponentType> rg_composed_program;
	//	diagnostic_blob.setNull();
	//	SLANG_CHECK("create program", compile_session->createCompositeComponentType(rg_component_types.data(), rg_component_types.size(), rg_composed_program.writeRef(), diagnostic_blob.writeRef()));
	//
	//	Slang::ComPtr<slang::IComponentType> rg_linked_program;
	//	diagnostic_blob.setNull();
	//	SLANG_CHECK("link program", rg_composed_program->link(rg_linked_program.writeRef(), diagnostic_blob.writeRef()));
	//
	//	Slang::ComPtr<slang::IBlob> rg_spirv_code;
	//	diagnostic_blob.setNull();
	//	SLANG_CHECK("get spirv code", rg_composed_program->getEntryPointCode(0, 0, rg_spirv_code.writeRef(), diagnostic_blob.writeRef()));
	//
	//	const VkShaderModuleCreateInfo rg_mod_ci = {
	//		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	//		.codeSize = rg_spirv_code->getBufferSize(),
	//		.pCode = reinterpret_cast<const uint32_t*>(rg_spirv_code->getBufferPointer()),
	//	};
	//
	//	VkShaderModule rg_mod = VK_NULL_HANDLE;
	//	VK_CHECK("create shader module", vkCreateShaderModule(vulkan_interface->GetDevice()->GetDevice(), &rg_mod_ci, nullptr, &rg_mod));
	//
	//	Slang::ComPtr<slang::IEntryPoint> ms_entry_point;
	//	slang_module->findEntryPointByName("miss", ms_entry_point.writeRef());
	//
	//	std::array<slang::IComponentType*, 2> ms_component_types = {
	//		slang_module, ms_entry_point
	//	};
	//
	//	Slang::ComPtr<slang::IComponentType> ms_composed_program;
	//	diagnostic_blob.setNull();
	//	SLANG_CHECK("create program", compile_session->createCompositeComponentType(ms_component_types.data(), ms_component_types.size(), ms_composed_program.writeRef(), diagnostic_blob.writeRef()));
	//
	//	Slang::ComPtr<slang::IComponentType> ms_linked_program;
	//	diagnostic_blob.setNull();
	//	SLANG_CHECK("link program", ms_composed_program->link(ms_linked_program.writeRef(), diagnostic_blob.writeRef()));
	//
	//	Slang::ComPtr<slang::IBlob> ms_spirv_code;
	//	diagnostic_blob.setNull();
	//	SLANG_CHECK("get spirv code", ms_composed_program->getEntryPointCode(0, 0, ms_spirv_code.writeRef(), diagnostic_blob.writeRef()));
	//
	//	const VkShaderModuleCreateInfo ms_mod_ci = {
	//		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	//		.codeSize = ms_spirv_code->getBufferSize(),
	//		.pCode = reinterpret_cast<const uint32_t*>(ms_spirv_code->getBufferPointer()),
	//	};
	//
	//	VkShaderModule ms_mod = VK_NULL_HANDLE;
	//	VK_CHECK("create shader module", vkCreateShaderModule(vulkan_interface->GetDevice()->GetDevice(), &ms_mod_ci, nullptr, &ms_mod));
	//
	//	Slang::ComPtr<slang::IEntryPoint> ch_entry_point;
	//	slang_module->findEntryPointByName("closesthit", ch_entry_point.writeRef());
	//
	//	std::array<slang::IComponentType*, 2> ch_component_types = {
	//		slang_module, ch_entry_point
	//	};
	//
	//	Slang::ComPtr<slang::IComponentType> ch_composed_program;
	//	diagnostic_blob.setNull();
	//	SLANG_CHECK("create program", compile_session->createCompositeComponentType(ch_component_types.data(), ch_component_types.size(), ch_composed_program.writeRef(), diagnostic_blob.writeRef()));
	//
	//	Slang::ComPtr<slang::IComponentType> ch_linked_program;
	//	diagnostic_blob.setNull();
	//	SLANG_CHECK("link program", ch_composed_program->link(ch_linked_program.writeRef(), diagnostic_blob.writeRef()));
	//
	//	Slang::ComPtr<slang::IBlob> ch_spirv_code;
	//	diagnostic_blob.setNull();
	//	SLANG_CHECK("get spirv code", ch_composed_program->getEntryPointCode(0, 0, ch_spirv_code.writeRef(), diagnostic_blob.writeRef()));
	//
	//	const VkShaderModuleCreateInfo ch_mod_ci = {
	//		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	//		.codeSize = ch_spirv_code->getBufferSize(),
	//		.pCode = reinterpret_cast<const uint32_t*>(ch_spirv_code->getBufferPointer()),
	//	};
	//
	//	VkShaderModule ch_mod = VK_NULL_HANDLE;
	//	VK_CHECK("create shader module", vkCreateShaderModule(vulkan_interface->GetDevice()->GetDevice(), &ch_mod_ci, nullptr, &ch_mod));

	std::filesystem::path rg_path = std::string(current_path).append("/shaders/glsl/raytrace.rgen.glsl.spv");
	VkShaderModule rg_mod = VK_NULL_HANDLE;
	if (std::filesystem::exists(rg_path))
	{
		std::uintmax_t file_size = std::filesystem::file_size(rg_path);
		std::ifstream rgen_file(rg_path.c_str(), std::ios::binary);

		std::vector<char> rg_code(file_size, 0);
		rgen_file.read(rg_code.data(), file_size);

		const VkShaderModuleCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = file_size,
			.pCode = reinterpret_cast<uint32_t*>(rg_code.data()),
		};
		VK_CHECK("create rgen module", vkCreateShaderModule(mDevice, &ci, nullptr, &rg_mod));
	}
	else
	{
		std::println("Could not find {}", rg_path.string());
	}

	std::filesystem::path ms_path = std::string(current_path).append("/shaders/glsl/raytrace.rmiss.glsl.spv");
	VkShaderModule ms_mod = VK_NULL_HANDLE;
	if (std::filesystem::exists(ms_path))
	{
		std::uintmax_t file_size = std::filesystem::file_size(ms_path);
		std::ifstream ms_file(ms_path.c_str(), std::ios::binary);

		std::vector<char> miss_code(file_size, 0);
		ms_file.read(miss_code.data(), file_size);

		const VkShaderModuleCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = file_size,
			.pCode = reinterpret_cast<uint32_t*>(miss_code.data()),
		};
		VK_CHECK("create rgen module", vkCreateShaderModule(mDevice, &ci, nullptr, &ms_mod));
	}
	else
	{
		std::println("Could not find {}", ms_path.string());
	}

	std::filesystem::path ch_path = std::string(current_path).append("/shaders/glsl/raytrace.rchit.glsl.spv");
	VkShaderModule ch_mod = VK_NULL_HANDLE;
	if (std::filesystem::exists(ch_path))
	{
		std::uintmax_t file_size = std::filesystem::file_size(ch_path);
		std::ifstream ch_file(ch_path.c_str(), std::ios::binary);

		std::vector<char> ch_code(file_size, 0);
		ch_file.read(ch_code.data(), file_size);

		const VkShaderModuleCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = file_size,
			.pCode = reinterpret_cast<uint32_t*>(ch_code.data()),
		};
		VK_CHECK("create rgen module", vkCreateShaderModule(mDevice, &ci, nullptr, &ch_mod));
	}
	else
	{
		std::println("Could not find {}", ch_path.string());
	}

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
		},
		{
			.binding = 3,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		},
		{
			.binding = 4,
			.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
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
			.size = sizeof(RaytracerPipelineData::PushConstants),
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

RaytracerPipelineData::~RaytracerPipelineData() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
	{
		for (auto& desc_set_layout : mDescriptorSetLayouts)
			vkDestroyDescriptorSetLayout(mDevice, desc_set_layout, nullptr);

		vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
		vkDestroyPipeline(mDevice, mPipeline, nullptr);
	}
}

VkPipeline RaytracerPipelineData::GetPipeline() const
{
	return mPipeline;
}

VkPipelineLayout RaytracerPipelineData::GetPipelineLayout() const
{
	return mPipelineLayout;
}

std::vector<VkDescriptorSetLayout> RaytracerPipelineData::GetDescriptorSetLayouts() const
{
	return mDescriptorSetLayouts;
}

std::vector<VkRayTracingShaderGroupCreateInfoKHR> RaytracerPipelineData::GetShaderGroups() const
{
	return mShaderGroups;
}

VulkanRaytracer::VulkanRaytracer(const VulkanInterface* const vulkan_interface, ImageResource* final_render_target, const VkExtent3D& extent, const std::string& current_path, const std::string& name)
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
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, vulkan_interface->GetAllocator()->GetAllocator(), mQueueFamilyIndices, "accum render target");
	mMaxFramesInFlight = static_cast<uint8_t>(vulkan_interface->GetSwapchain()->GetImagesCount());
	mExtent = extent;
	mComputeQueue = vulkan_interface->GetDevice()->GetComputeQueue();
	mTransferQueue = vulkan_interface->GetTransferObjects()->GetQueue();
	mTransferCommandBuffer = vulkan_interface->GetTransferObjects()->GetCommandBuffer();
	mFrameObjects = std::make_unique<FrameObjects>(mDevice, vulkan_interface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex, mMaxFramesInFlight, "raytrace frame objects");
	std::vector<uint8_t> uniform_buffer_data(sizeof(glm::mat4) * 2);
	mUniformBuffer = std::make_unique<BufferResource>(mDevice, mAllocator, uniform_buffer_data,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		"uniform buffer");
	mPipelineData = std::make_unique<RaytracerPipelineData>(vulkan_interface, current_path, "reytrace pipeline data");

	mDescriptorSets.resize(mMaxFramesInFlight);

	const uint32_t aligned_handle_size = static_cast<uint32_t>(ALIGNED_SIZE(mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment));
	std::vector<uint8_t> sbt_handle_data(aligned_handle_size);
	mRaygenSBT = std::make_unique<BufferResource>(
		mDevice, mAllocator, sbt_handle_data,
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		 "rb sbt");
	mMissSBT = std::make_unique<BufferResource>(
		mDevice, mAllocator, sbt_handle_data,
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		 "rb sbt");
	mCHSBT = std::make_unique<BufferResource>(
		mDevice, mAllocator, sbt_handle_data,
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		 "rb sbt");

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

VulkanRaytracer::~VulkanRaytracer()
{
	if (mDevice != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
}

void VulkanRaytracer::InitializeResources()
{
	const VkCommandBufferBeginInfo cmd_buff_bi = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};

	VK_CHECK("begin xfer cmd buff", vkBeginCommandBuffer(mTransferCommandBuffer, &cmd_buff_bi));

	Utils_ChangeImageLayout(
		mTransferCommandBuffer,
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		VK_IMAGE_ASPECT_COLOR_BIT,
		mAccumRenderTarget->GetImage()
	);

	VK_CHECK("end xfer cmd buff", vkEndCommandBuffer(mTransferCommandBuffer));

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = mTransferCommandBuffer,
		},
	};

	const VkSubmitInfo2 submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = std::size(cmd_buff_infos),
			.pCommandBufferInfos = cmd_buff_infos,
		},
	};

	VK_CHECK("submit geom buffer xfer cmd", vkQueueSubmit2KHR(mTransferQueue, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));
	VK_CHECK("wait xfer cmd buff", vkQueueWaitIdle(mTransferQueue));

	std::vector<uint32_t> rand_states(4 * mExtent.width * mExtent.height);

	for (uint32_t st = 0; st < 4 * mExtent.width * mExtent.height; ++st)
	{
		uint32_t rand_val = rand();
		while (rand_val < 128)
			rand_val = rand();

		rand_states[st] = rand_val;
	}

	std::vector<uint8_t>rand_states_data(rand_states.size() * sizeof(uint32_t));
	std::memcpy(rand_states_data.data(), rand_states.data(), rand_states_data.size());

	mRandomStates = std::make_unique<BufferResource>(
		mDevice, mAllocator, rand_states_data,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		0, "rand states", mTransferCommandBuffer, mTransferQueue);

	const uint32_t aligned_handle_size = static_cast<uint32_t>(ALIGNED_SIZE(mRayTracingProperties.shaderGroupHandleSize, mRayTracingProperties.shaderGroupHandleAlignment));
	const uint32_t sbt_size = aligned_handle_size * static_cast<uint32_t>(std::size(mPipelineData->GetShaderGroups()));

	std::vector<uint8_t> shader_handle_storage(sbt_size);
	VK_CHECK("get rt shader handles", vkGetRayTracingShaderGroupHandlesKHR(mDevice, mPipelineData->GetPipeline(), 0, static_cast<uint32_t>(std::size(mPipelineData->GetShaderGroups())), sbt_size, shader_handle_storage.data()));

	memcpy(mRaygenSBT->GetAllocationInfo2().allocationInfo.pMappedData, shader_handle_storage.data(), sbt_size);
	memcpy(mMissSBT->GetAllocationInfo2().allocationInfo.pMappedData, shader_handle_storage.data() + mRayTracingProperties.shaderGroupHandleSize, sbt_size);
	memcpy(mCHSBT->GetAllocationInfo2().allocationInfo.pMappedData, shader_handle_storage.data() + (mRayTracingProperties.shaderGroupHandleSize * 2), sbt_size);

	auto proj = glm::perspective(glm::radians(135.f), 1.77f, 0.001f, 100.f);
	proj[1][1] *= -1;

	glm::highp_mat4 mats[2] = {
		glm::inverse(glm::lookAt(glm::vec3(0.f,0.f,1.f), glm::vec3(0,0,0), glm::vec3(0,1,0))),
		glm::inverse(proj),
	};

	memcpy(mUniformBuffer->GetAllocationInfo2().allocationInfo.pMappedData, mats, sizeof(mats));
}

void VulkanRaytracer::RecreateRenderResources(const VkExtent2D& extent)
{
	mExtent = { extent.width , extent.height, 1 };
	mAccumRenderTarget = std::make_unique<ImageResource>(mDevice, mExtent, VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, mAllocator,
		mQueueFamilyIndices, "accum render target");

	InitializeResources();
}

void VulkanRaytracer::Start(const VulkanRaytracerScene* scene, const uint32_t max_samples)
{
	VkDevice device = mDevice;
	VkCommandBuffer cmd_buff = mFrameObjects->GetCommandBuffer();
	VkSemaphore frame_sem = mFrameObjects->GetSemaphore();
	uint64_t& frame_sem_value = mFrameObjects->GetFrameSemValue();
	uint8_t frame_in_flight = mFrameObjects->GetFrameInFlight();

	struct Vertex {
		float pos[3];
	};

	const Vertex vertices[] = {
	{{  1.0f,  1.0f, 0.0f }},
	{{ -1.0f,  1.0f, 0.0f }},
	{{  0.0f, -1.0f, 0.0f }}
	};

	size_t vertices_size = sizeof(vertices);

	std::vector<uint8_t>vertices_data(vertices_size);
	std::memcpy(vertices_data.data(), vertices, vertices_size);

	uint32_t indices[] = { 0, 1, 2 };
	size_t indices_size = sizeof(indices);

	std::vector<uint8_t>indices_data(indices_size);
	std::memcpy(indices_data.data(), indices, indices_size);

	VkTransformMatrixKHR blas_transform = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0
	};
	std::vector<uint8_t> blas_transform_data(sizeof(VkTransformMatrixKHR));
	std::memcpy(blas_transform_data.data(), &blas_transform, sizeof(VkTransformMatrixKHR));

	auto vertices_buffer = std::make_unique<BufferResource>(mDevice, mAllocator, vertices_data,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		 "vertices buffer");

	auto indices_buffer = std::make_unique<BufferResource>(mDevice, mAllocator, indices_data,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		 "indices buffer");

	auto transform_buffer = std::make_unique<BufferResource>(mDevice, mAllocator, blas_transform_data,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		 "blas transform buffer");

	const VkAccelerationStructureGeometryKHR blas_geom = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
		.geometry = {
			.triangles = {
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
				.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
				.vertexData = vertices_buffer->GetDeviceOrHostAddressConstKHR(),
				.vertexStride = sizeof(Vertex),
				.maxVertex = 2,
				.indexType = VK_INDEX_TYPE_UINT32,
				.indexData = indices_buffer->GetDeviceOrHostAddressConstKHR(),
				.transformData = transform_buffer->GetDeviceOrHostAddressConstKHR(),
			},
		},
	};

	VkAccelerationStructureBuildGeometryInfoKHR blas_build_geom_info = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.geometryCount = 1,
		.pGeometries = &blas_geom,
	};

	uint32_t max_triangles = 1;

	VkAccelerationStructureBuildSizesInfoKHR blas_size_info = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
	};

	vkGetAccelerationStructureBuildSizesKHR(mDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&blas_build_geom_info, &max_triangles, &blas_size_info);

	std::vector<uint8_t> blas_buffer_data(blas_size_info.accelerationStructureSize);
	auto blas_buffer = std::make_unique<BufferResource>(mDevice, mAllocator, blas_buffer_data,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		0, "blas buffer", mTransferCommandBuffer, mTransferQueue);

	std::vector<uint8_t> blas_scratch_buffer_data(blas_size_info.buildScratchSize);
	auto blas_scratch_buffer = std::make_unique<BufferResource>(mDevice, mAllocator, blas_scratch_buffer_data,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		0, "blas scratch buffer", mTransferCommandBuffer, mTransferQueue);

	VkAccelerationStructureKHR blas = VK_NULL_HANDLE;
	const VkAccelerationStructureCreateInfoKHR blas_create_info = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = blas_buffer->GetDescriptorInfo().buffer,
		.size = blas_size_info.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
	};

	VK_CHECK("create blas", vkCreateAccelerationStructureKHR(mDevice, &blas_create_info, nullptr, &blas));

	const VkSemaphoreWaitInfo wait_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		.semaphoreCount = 1,
		.pSemaphores = &frame_sem,
		.pValues = &frame_sem_value,
	};

	VK_CHECK("wait blas sem", vkWaitSemaphoresKHR(device, &wait_info, UINT64_MAX));

	const VkCommandBufferBeginInfo blas_begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VK_CHECK("begin blas cmd_buff", vkBeginCommandBuffer(cmd_buff, &blas_begin_info));

	VkAccelerationStructureBuildRangeInfoKHR blas_build_range_info = {
		.primitiveCount = 1,
	};

	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> blas_build_range_infos = {
		&blas_build_range_info
	};

	blas_build_geom_info.dstAccelerationStructure = blas;
	blas_build_geom_info.scratchData = blas_scratch_buffer->GetDeviceOrHostAddressKHR();

	vkCmdBuildAccelerationStructuresKHR(cmd_buff, 1, &blas_build_geom_info, blas_build_range_infos.data());

	VK_CHECK("end blas cmd_buff", vkEndCommandBuffer(cmd_buff));

	const VkCommandBufferSubmitInfoKHR blas_cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
			.commandBuffer = cmd_buff,
		}
	};

	const VkSemaphoreSubmitInfoKHR blas_sig_sem_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
			.semaphore = frame_sem,
			.value = ++frame_sem_value,
			.stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT_KHR,
		}
	};

	const VkSubmitInfo2KHR blas_submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
			.commandBufferInfoCount = std::size(blas_cmd_buff_infos),
			.pCommandBufferInfos = blas_cmd_buff_infos,
			.signalSemaphoreInfoCount = std::size(blas_sig_sem_infos),
			.pSignalSemaphoreInfos = blas_sig_sem_infos,
		},
	};

	VK_CHECK("submit blas", vkQueueSubmit2KHR(mComputeQueue, std::size(blas_submit_infos), blas_submit_infos, VK_NULL_HANDLE));

	const VkAccelerationStructureDeviceAddressInfoKHR blas_addr_info = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
		.accelerationStructure = blas,
	};

	const VkTransformMatrixKHR tlas_transform = blas_transform;
	const VkAccelerationStructureInstanceKHR tlas_instance = {
		.transform = tlas_transform,
		.mask = 0xFF,
		.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
		.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(mDevice, &blas_addr_info),
	};

	std::vector<uint8_t>tlas_instance_data(sizeof(VkAccelerationStructureInstanceKHR));
	std::memcpy(tlas_instance_data.data(), &tlas_instance, sizeof(VkAccelerationStructureInstanceKHR));

	auto instance_buffer = std::make_unique<BufferResource>(mDevice, mAllocator, tlas_instance_data,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		"instance buffer");

	const VkAccelerationStructureGeometryKHR tlas_geom = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
		.geometry = {
			.instances = {
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
				.data = instance_buffer->GetDeviceOrHostAddressConstKHR(),
			},
		},
	};

	VkAccelerationStructureBuildGeometryInfoKHR tlas_build_geom_info = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.geometryCount = 1,
		.pGeometries = &tlas_geom,
	};

	uint32_t primitive_count = 1;

	VkAccelerationStructureBuildSizesInfoKHR tlas_size_info = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
	};

	vkGetAccelerationStructureBuildSizesKHR(mDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&tlas_build_geom_info, &primitive_count, &tlas_size_info);

	std::vector<uint8_t> tlas_buffer_data(tlas_size_info.accelerationStructureSize);
	auto tlas_buffer = std::make_unique<BufferResource>(mDevice, mAllocator, tlas_buffer_data,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		0, "tlas buffer", mTransferCommandBuffer, mTransferQueue);

	std::vector<uint8_t> tlas_scratch_buffer_data(tlas_size_info.buildScratchSize);
	auto tlas_scratch_buffer = std::make_unique<BufferResource>(mDevice, mAllocator, tlas_scratch_buffer_data,
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		0, "tlas scratch buffer", mTransferCommandBuffer, mTransferQueue);

	const VkAccelerationStructureCreateInfoKHR tlas_create_info = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = tlas_buffer->GetDescriptorInfo().buffer,
		.size = tlas_size_info.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
	};

	VkAccelerationStructureKHR tlas = VK_NULL_HANDLE;
	VK_CHECK("create tlas", vkCreateAccelerationStructureKHR(mDevice, &tlas_create_info, nullptr, &tlas));

	VK_CHECK("wait tlas sem", vkWaitSemaphoresKHR(device, &wait_info, UINT64_MAX));

	const VkCommandBufferBeginInfo tlas_begin_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VK_CHECK("begin tlas cmd_buff", vkBeginCommandBuffer(cmd_buff, &tlas_begin_info));

	VkAccelerationStructureBuildRangeInfoKHR tlas_build_range_info = {
		.primitiveCount = primitive_count,
	};

	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> tlas_build_range_infos = {
		&tlas_build_range_info
	};

	tlas_build_geom_info.dstAccelerationStructure = tlas;
	tlas_build_geom_info.scratchData = tlas_scratch_buffer->GetDeviceOrHostAddressKHR();

	vkCmdBuildAccelerationStructuresKHR(cmd_buff, 1, &tlas_build_geom_info, tlas_build_range_infos.data());

	VK_CHECK("end tlas cmd_buff", vkEndCommandBuffer(cmd_buff));

	const VkCommandBufferSubmitInfoKHR tlas_cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
			.commandBuffer = cmd_buff,
		}
	};

	const VkSemaphoreSubmitInfoKHR tlas_sig_sem_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
			.semaphore = frame_sem,
			.value = ++frame_sem_value,
			.stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT_KHR,
		}
	};

	const VkSubmitInfo2KHR tlas_submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
			.commandBufferInfoCount = std::size(tlas_cmd_buff_infos),
			.pCommandBufferInfos = tlas_cmd_buff_infos,
			.signalSemaphoreInfoCount = std::size(tlas_sig_sem_infos),
			.pSignalSemaphoreInfos = tlas_sig_sem_infos,
		},
	};

	VK_CHECK("submit tlas", vkQueueSubmit2KHR(mComputeQueue, std::size(tlas_submit_infos), tlas_submit_infos, VK_NULL_HANDLE));

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

		const VkDescriptorImageInfo accum_target_desc_info = mAccumRenderTarget->GetDescriptorInfo();
		const VkDescriptorImageInfo final_render_desc_info = mFinalRenderTarget->GetDescriptorInfo();
		const VkDescriptorBufferInfo rand_states_desc_info = mRandomStates->GetDescriptorInfo();
		const VkDescriptorBufferInfo uniform_buff_desc_info = mUniformBuffer->GetDescriptorInfo();
		const VkWriteDescriptorSetAccelerationStructureKHR tlas_desc_info = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
			.accelerationStructureCount = 1,
			.pAccelerationStructures = &tlas,
		};

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
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = mDescriptorSets[frame_in_flight],
				.dstBinding = 3,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &uniform_buff_desc_info,
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = &tlas_desc_info,
				.dstSet = mDescriptorSets[frame_in_flight],
				.dstBinding = 4,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
			}
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

		const RaytracerPipelineData::PushConstants rt_pc = {
			.CurrentSample = s,
		};

		const VkPushConstantsInfo rt_pc_info = {
			.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO,
			.layout = mPipelineData->GetPipelineLayout(),
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			.size = sizeof(RaytracerPipelineData::PushConstants),
			.pValues = &rt_pc,
		};

		vkCmdPushConstants2KHR(cmd_buff, &rt_pc_info);

		const VkStridedDeviceAddressRegionKHR rg_sbt = {
			.deviceAddress = mRaygenSBT->GetDeviceAddress(),
			.stride = mRayTracingProperties.shaderGroupHandleSize,
			.size = mRayTracingProperties.shaderGroupHandleSize,
		};

		const VkStridedDeviceAddressRegionKHR ms_sbt = {
			.deviceAddress = mMissSBT->GetDeviceAddress(),
			.stride = mRayTracingProperties.shaderGroupHandleSize,
			.size = mRayTracingProperties.shaderGroupHandleSize,
		};
		const VkStridedDeviceAddressRegionKHR ch_sbt = {
			.deviceAddress = mCHSBT->GetDeviceAddress(),
			.stride = mRayTracingProperties.shaderGroupHandleSize,
			.size = mRayTracingProperties.shaderGroupHandleSize,
		};

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

	mStopRendering = false;

	vkDestroyAccelerationStructureKHR(mDevice, tlas, nullptr);
	vkDestroyAccelerationStructureKHR(mDevice, blas, nullptr);

	SDL_CHECK(SDL_PushEvent(&events.RaytraceStopped));
}

void VulkanRaytracer::UpdateFinalRenderTarget(ImageResource* FinalRenderTarget)
{
	mFinalRenderTarget = FinalRenderTarget;
}

void VulkanRaytracer::Stop()
{
	mStopRendering = true;
}

