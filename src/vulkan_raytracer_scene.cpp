#include "vulkan_raytracer_scene.hpp"
#include "vulkan_objects.hpp"
#include "resources.hpp"
#include "utils.hpp"
#include "vulkan_interface.hpp"

#include <stb_image.h>

class VulkanRaytracerScenePipelineData
{
public:
	VulkanRaytracerScenePipelineData() = delete;
	VulkanRaytracerScenePipelineData(const VkDevice device, const std::string& current_path, const size_t num_images, const std::string& name);

	VulkanRaytracerScenePipelineData(const VulkanRaytracerScenePipelineData& other) = delete;
	VulkanRaytracerScenePipelineData& operator=(const VulkanRaytracerScenePipelineData& other) = delete;

	~VulkanRaytracerScenePipelineData() noexcept;

	struct PushConstants
	{
		uint32_t CurrentSample = 1;
	};

	VkPipeline GetPipeline() const;
	VkPipelineLayout GetPipelineLayout() const;
	const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const;
	const std::vector<VkRayTracingShaderGroupCreateInfoKHR>& GetShaderGroups() const;

private:
	VkPipeline mPipeline = VK_NULL_HANDLE;
	VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> mShaderGroups;

	VkDevice mDevice = VK_NULL_HANDLE;
};

VulkanRaytracerScenePipelineData::VulkanRaytracerScenePipelineData(const VkDevice device, const std::string& current_path, const size_t num_images, const std::string& name)
	: mDevice(device)
{
	Slang::ComPtr<slang::IGlobalSession> slang_global_session;
	SLANG_CHECK("create global session", slang::createGlobalSession(slang_global_session.writeRef()));

	const slang::TargetDesc target_descs[] = {
		{
			.format = SLANG_SPIRV,
			.profile = slang_global_session->findProfile("spirv_1_5"),
		}
	};

	slang::CompilerOptionEntry compiler_options[] = {
		{
			.name = slang::CompilerOptionName::MatrixLayoutColumn,
			.value = {
				.kind = slang::CompilerOptionValueKind::Int,
				.intValue0 = 1,
			},
		},
		{
			.name = slang::CompilerOptionName::DisableWarnings,
			.value = {
				.kind = slang::CompilerOptionValueKind::String,
				.stringValue0 = "41012"
			},
		},
#ifdef _DEBUG
		{
			.name = slang::CompilerOptionName::DebugInformation,
			.value = {
				.kind = slang::CompilerOptionValueKind::Int,
				.intValue0 = SLANG_DEBUG_INFO_LEVEL_MAXIMAL,
			}
		},
#else	// _DEBUG
		{
			.name = slang::CompilerOptionName::Optimization,
			.value = {
				.kind = slang::CompilerOptionValueKind::Int,
				.intValue0 = SLANG_OPTIMIZATION_LEVEL_MAXIMAL
			},
		},
#endif // _DEBUG
	};

	slang::SessionDesc compile_session_desc = {
		.targets = target_descs,
		.targetCount = std::size(target_descs),
		.compilerOptionEntries = compiler_options,
		.compilerOptionEntryCount = std::size(compiler_options),
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
	VK_CHECK("create shader module", vkCreateShaderModule(device, &rg_mod_ci, nullptr, &rg_mod));

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
	VK_CHECK("create shader module", vkCreateShaderModule(device, &ms_mod_ci, nullptr, &ms_mod));

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
	VK_CHECK("create shader module", vkCreateShaderModule(device, &ch_mod_ci, nullptr, &ch_mod));

	/*std::filesystem::path rg_path = std::string(current_path).append("/shaders/glsl/raytrace.rgen.glsl.spv");
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
	}*/

	const VkDescriptorSetLayoutBinding dsl_0_bindings[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		},
	};

	const VkDescriptorSetLayoutBinding dsl_1_bindings[] = {
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
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
		},
		{
			.binding = 3,
			.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		},
		{
			.binding = 4,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
		},
		{
			.binding = 5,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = static_cast<uint32_t>(num_images),
			.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
		},
	};

	const VkDescriptorBindingFlagsEXT binding_flags[] = {
		0, 0, 0, 0, 0, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT
	};

	const VkDescriptorSetLayoutBindingFlagsCreateInfo dsl_1_binds_flags_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.bindingCount = std::size(binding_flags),
		.pBindingFlags = binding_flags,
	};

	const VkDescriptorSetLayoutCreateInfo dsl_cis[] = {
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = std::size(dsl_0_bindings),
			.pBindings = dsl_0_bindings,
		},
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = &dsl_1_binds_flags_ci,
			.bindingCount = std::size(dsl_1_bindings),
			.pBindings = dsl_1_bindings,
		},
	};

	mDescriptorSetLayouts.resize(std::size(dsl_cis));

	for (size_t dsl_ci = 0; dsl_ci < std::size(dsl_cis); ++dsl_ci)
	{
		VK_CHECK("create desc set layout", vkCreateDescriptorSetLayout(device, &dsl_cis[dsl_ci], nullptr, &mDescriptorSetLayouts[dsl_ci]));
#ifdef _DEBUG
		Utils_SetObjectName(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, reinterpret_cast<uint64_t>(mDescriptorSetLayouts[dsl_ci]), "vulkan raytrace scene desc set");
#endif // _DEBUG
	}

	const VkPushConstantRange pc_ranges[] = {
		{
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			.size = sizeof(VulkanRaytracerScenePipelineData::PushConstants),
		},
	};

	const VkPipelineLayoutCreateInfo pl_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = static_cast<uint32_t>(mDescriptorSetLayouts.size()),
		.pSetLayouts = mDescriptorSetLayouts.data(),
		.pushConstantRangeCount = std::size(pc_ranges),
		.pPushConstantRanges = pc_ranges,
	};

	VK_CHECK("create pipeline layout", vkCreatePipelineLayout(device, &pl_ci, nullptr, &mPipelineLayout));

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

	VK_CHECK("create rt pipeline", vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, std::size(create_infos), create_infos, nullptr, &mPipeline));

	vkDestroyShaderModule(device, rg_mod, nullptr);
	vkDestroyShaderModule(device, ch_mod, nullptr);
	vkDestroyShaderModule(device, ms_mod, nullptr);

#ifdef _DEBUG
	Utils_SetObjectName(device, VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(mPipeline), std::string(name).append(" pipeline").c_str());
	Utils_SetObjectName(device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, reinterpret_cast<uint64_t>(mPipelineLayout), std::string(name).append(" pipeline layout").c_str());

	for (size_t dsl = 0; dsl < mDescriptorSetLayouts.size(); ++dsl)
	{
		Utils_SetObjectName(device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, reinterpret_cast<uint64_t>(mDescriptorSetLayouts[dsl]), std::string(name).append(" descriptor set layout ").append(std::to_string(dsl)).c_str());
	}
#endif	// _DEBUG
}

VulkanRaytracerScenePipelineData::~VulkanRaytracerScenePipelineData() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
	{
		for (auto& desc_set_layout : mDescriptorSetLayouts)
			vkDestroyDescriptorSetLayout(mDevice, desc_set_layout, nullptr);

		vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
		vkDestroyPipeline(mDevice, mPipeline, nullptr);
	}
}

VkPipeline VulkanRaytracerScenePipelineData::GetPipeline() const
{
	return mPipeline;
}

VkPipelineLayout VulkanRaytracerScenePipelineData::GetPipelineLayout() const
{
	return mPipelineLayout;
}

const std::vector<VkDescriptorSetLayout>& VulkanRaytracerScenePipelineData::GetDescriptorSetLayouts() const
{
	return mDescriptorSetLayouts;
}

const std::vector<VkRayTracingShaderGroupCreateInfoKHR>& VulkanRaytracerScenePipelineData::GetShaderGroups() const
{
	return mShaderGroups;
}

VulkanRaytracerScene::VulkanRaytracerScene(const Scene& scene, const VkDevice device, const VmaAllocator allocator, const std::vector<uint32_t>& queue_family_indices, const std::string& current_path, const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracing_properties, const size_t scratch_buffer_alignment, ComputeHelpers* compute_helpers)
	: mDevice(device), mRaytracingProperties(raytracing_properties)
{
	mPipelineData = std::make_unique<VulkanRaytracerScenePipelineData>(device, current_path, std::max(static_cast<size_t>(1), scene.GetImages().size()), "vulkan raytracer");

	const VkDeviceSize sbt_handle_size = raytracing_properties.shaderGroupHandleSize;
	const VkDeviceSize sbt_handle_aligned_size = ALIGNED_SIZE(raytracing_properties.shaderGroupHandleSize, raytracing_properties.shaderGroupHandleAlignment);
	const size_t group_count = mPipelineData->GetShaderGroups().size();
	const size_t sbt_size = group_count * sbt_handle_size;

	std::vector<uint8_t> sbt_handles(sbt_size);
	VK_CHECK("get sbt group handles", vkGetRayTracingShaderGroupHandlesKHR(device, mPipelineData->GetPipeline(), 0, static_cast<uint32_t>(group_count), sbt_size, sbt_handles.data()));

	auto staging_rg_sbt = std::make_unique<HostBufferResource>(
		device, allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		ALIGNED_SIZE(sbt_handle_aligned_size, raytracing_properties.shaderGroupBaseAlignment ), "staging rg sbt"
	);
	std::memcpy(staging_rg_sbt->GetAllocationInfo2().allocationInfo.pMappedData, sbt_handles.data(), sbt_handle_size);

	mRGSbt = std::make_unique<DeviceBufferResource>(
		device, allocator, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		ALIGNED_SIZE(sbt_handle_aligned_size, raytracing_properties.shaderGroupBaseAlignment), "rg sbt"
	);

	auto staging_ms_sbt = std::make_unique<HostBufferResource>(
		device, allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		ALIGNED_SIZE(sbt_handle_aligned_size, raytracing_properties.shaderGroupBaseAlignment), "staging ms sbt"
	);
	std::memcpy(staging_ms_sbt->GetAllocationInfo2().allocationInfo.pMappedData, sbt_handles.data() + sbt_handle_aligned_size, sbt_handle_size);

	mMSSbt = std::make_unique<DeviceBufferResource>(
		device, allocator, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		ALIGNED_SIZE(sbt_handle_aligned_size, raytracing_properties.shaderGroupBaseAlignment), "ms sbt"
	);

	// current supporting one material per primitive, one primitive per mesh
	for (const auto& mesh : scene.GetMeshes())
	{
		mBLASes.push_back(
			std::make_unique<BLAccelerationStructure>(
				device, allocator, mesh.GetPrimitives()[0], scene.GetVertexData(), scratch_buffer_alignment, compute_helpers, "BLAS"
			)
		);
	}

	std::vector<VkAccelerationStructureInstanceKHR> instances;
	instances.reserve(scene.GetMeshInstances().size());

	struct CHSbtRecordData
	{
		uint32_t material_index;
	};

	std::vector<uint8_t> ch_sbt;
	mCHSbtRecordAlignedSize = ALIGNED_SIZE(raytracing_properties.shaderGroupHandleSize + sizeof(CHSbtRecordData), raytracing_properties.shaderGroupHandleAlignment);
	ch_sbt.reserve(ALIGNED_SIZE(scene.GetMaterials().size() * mCHSbtRecordAlignedSize, raytracing_properties.shaderGroupBaseAlignment));

	for (const auto& mesh_instance : scene.GetMeshInstances())
	{
		const VkAccelerationStructureDeviceAddressInfoKHR blas_addr_info = {
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
			.accelerationStructure = mBLASes[mesh_instance.GetMeshIndex()]->GetAS(),
		};

		glm::mat4 xform = {};
		std::memcpy(&xform, scene.GetUniformData().data() + mesh_instance.GetModelMatrixOffset(), sizeof(xform));

		xform = glm::transpose(xform);
		VkTransformMatrixKHR inst_xform;
		std::memcpy(&inst_xform, &xform, sizeof(inst_xform));

		VkAccelerationStructureInstanceKHR inst = {
			.transform = inst_xform,
			.mask = 0xFF,
			.instanceShaderBindingTableRecordOffset = scene.GetMeshes()[mesh_instance.GetMeshIndex()].GetPrimitives()[0].GetMaterialIndex(),
			.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(device, &blas_addr_info),
		};

		CHSbtRecordData ch_sbt_record = {
			.material_index = scene.GetMeshes()[mesh_instance.GetMeshIndex()].GetPrimitives()[0].GetMaterialIndex(),
		};

		std::vector<uint8_t> ch_sbt_record_data(mCHSbtRecordAlignedSize);
		std::memcpy(ch_sbt_record_data.data(), sbt_handles.data() + (sbt_handle_size * 2), sbt_handle_size);
		std::memcpy(ch_sbt_record_data.data() + raytracing_properties.shaderGroupHandleSize, &ch_sbt_record, sizeof(CHSbtRecordData));

		ch_sbt.append_range(ch_sbt_record_data);

		instances.push_back(inst);
	}

	mCHSbtAlignedSize = ALIGNED_SIZE(ch_sbt.size(), raytracing_properties.shaderGroupBaseAlignment);

	auto staging_ch_sbt = std::make_unique<HostBufferResource>(
		device, allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		mCHSbtAlignedSize, "staging ch sbt"
	);
	std::memcpy(staging_ch_sbt->GetAllocationInfo2().allocationInfo.pMappedData, ch_sbt.data(), ch_sbt.size());

	mCHSbt = std::make_unique<DeviceBufferResource>(
		device, allocator, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		mCHSbtAlignedSize, "ch sbt"
	);

	mTLAS = std::make_unique<TLAccelerationStructure>(
		device, allocator, instances, scratch_buffer_alignment, compute_helpers, "TLAS"
	);

	auto uniform_data = scene.GetUniformData();

	mUniformData = std::make_unique<DeviceBufferResource>(
		device, allocator,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		uniform_data.size(), "scene uniform data");

	auto staging_uniform_data = std::make_unique<HostBufferResource>(
		device, allocator,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		uniform_data, "staging uniform data"
	);

	const VkDescriptorPoolSize pool_sizes[] = {
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.descriptorCount = 1,
		},
		{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 2,
		},
		{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
		},
		{
			.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
			.descriptorCount = 1,
		},
		{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
		},
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = static_cast<uint32_t>(std::max(static_cast<size_t>(1), scene.GetImages().size())),
		},
	};

	uint32_t max_sets = 0;
	std::for_each(std::begin(pool_sizes), std::end(pool_sizes), [&max_sets](const VkDescriptorPoolSize ps) { max_sets += ps.descriptorCount; });

	const VkDescriptorPoolCreateInfo dsp_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = max_sets,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes,
	};

	VK_CHECK("create descriptor pool", vkCreateDescriptorPool(mDevice, &dsp_ci, nullptr, &mDescriptorPool));

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_DESCRIPTOR_POOL, reinterpret_cast<uint64_t>(mDescriptorPool), "vulkan raytracer scene descriptor pool");
#endif // _DEBUG

	const std::vector<VkDescriptorSetLayout>& desc_set_layouts = mPipelineData->GetDescriptorSetLayouts();

	const VkDescriptorSetAllocateInfo cam_ds_ai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = mDescriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = desc_set_layouts.data(),
	};

	VK_CHECK("allocate desc set", vkAllocateDescriptorSets(device, &cam_ds_ai, &mCameraDescSet));

#ifdef _DEBUG
	Utils_SetObjectName(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(mCameraDescSet), "vulkan raytracer cam descriptor set");
#endif // _DEBUG

	const uint32_t tex_desc_counts[] = {
		static_cast<uint32_t>(std::max(static_cast<size_t>(1), scene.GetImages().size()))
	};

	const VkDescriptorSetVariableDescriptorCountAllocateInfoEXT tex_ds_vdcai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
		.descriptorSetCount = std::size(tex_desc_counts),
		.pDescriptorCounts = tex_desc_counts,
	};

	const VkDescriptorSetAllocateInfo ds_ai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = &tex_ds_vdcai,
		.descriptorPool = mDescriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = desc_set_layouts.data() + 1,
	};

	VK_CHECK("allocate desc set", vkAllocateDescriptorSets(device, &ds_ai, &mSceneDescSet));

#ifdef _DEBUG
	Utils_SetObjectName(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(mSceneDescSet), "vulkan raytracer scene descriptor set");
#endif // _DEBUG

	mCameraDescBuffer = {
		.buffer = mUniformData->GetVkBuffer(),
		.range = sizeof(glm::mat4) * 2,
	};

	mCameraInstances.reserve(scene.GetCameraInstances().size());
	for (const auto& camera_instance : scene.GetCameraInstances())
	{
		mCameraInstances.push_back(VulkanRaytracerScene::CameraInstance(camera_instance));
	}

	mMaterials.reserve(scene.GetMaterials().size());
	for (const auto& material : scene.GetMaterials())
	{
		mMaterials.push_back(VulkanRaytracerScene::Material(material));
	}

	std::vector<uint8_t> materials_data(mMaterials.size() * sizeof(Material));
	std::memcpy(materials_data.data(), mMaterials.data(), materials_data.size());

	auto staging_materials_data = std::make_unique<HostBufferResource>(
		device, allocator,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		materials_data, "staging materials"
	);

	mMaterialsBuffer = std::make_unique<DeviceBufferResource>(device, allocator,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		mMaterials.size() * sizeof(Material), "materials"
	);

	const VkDescriptorBufferInfo mat_desc_buff = {
		.buffer = mMaterialsBuffer->GetVkBuffer(),
		.range = VK_WHOLE_SIZE,
	};

	mImages.reserve(scene.GetImages().size());
	for (const auto& image : scene.GetImages())
	{
		mImages.push_back(VulkanRaytracerScene::Image(image, scene.GetImagesData(), device, allocator, queue_family_indices, compute_helpers));
	}

	if (mImages.size() == 0)
	{
		mImages.push_back(VulkanRaytracerScene::Image(std::string(current_path).append("/images/one_pix.jpg").c_str(), device, allocator, queue_family_indices, compute_helpers));
	}

	std::vector<VkDescriptorImageInfo> image_descs;
	image_descs.reserve(mImages.size());

	for (const auto& image : mImages)
	{
		image_descs.push_back(image.GetImageResource()->GetDescriptorInfo());
	}

	VkAccelerationStructureKHR tlas = mTLAS->GetAS();
	const VkWriteDescriptorSetAccelerationStructureKHR tlas_desc_info = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
		.accelerationStructureCount = 1,
		.pAccelerationStructures = &tlas,
	};

	const VkWriteDescriptorSet ds_writes[] = {
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = mCameraDescSet,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.pBufferInfo = &mCameraDescBuffer,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = &tlas_desc_info,
			.dstSet = mSceneDescSet,
			.dstBinding = 3,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = mSceneDescSet,
			.dstBinding = 4,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &mat_desc_buff,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = mSceneDescSet,
			.dstBinding = 5,
			.descriptorCount = static_cast<uint32_t>(image_descs.size()),
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = image_descs.data(),
		},
	};

	vkUpdateDescriptorSets(device, std::size(ds_writes), ds_writes, 0, nullptr);

	compute_helpers->RecordBatch();
	compute_helpers->CopyBufferToBuffer(staging_uniform_data->GetVkBuffer(), mUniformData->GetVkBuffer(), uniform_data.size());
	compute_helpers->CopyBufferToBuffer(staging_materials_data->GetVkBuffer(), mMaterialsBuffer->GetVkBuffer(), materials_data.size());
	compute_helpers->CopyBufferToBuffer(staging_rg_sbt->GetVkBuffer(), mRGSbt->GetVkBuffer(), staging_rg_sbt->GetAllocationInfo2().allocationInfo.size);
	compute_helpers->CopyBufferToBuffer(staging_ms_sbt->GetVkBuffer(), mMSSbt->GetVkBuffer(), staging_ms_sbt->GetAllocationInfo2().allocationInfo.size);
	compute_helpers->CopyBufferToBuffer(staging_ch_sbt->GetVkBuffer(), mCHSbt->GetVkBuffer(), staging_ch_sbt->GetAllocationInfo2().allocationInfo.size);
	compute_helpers->SubmitBatch();
}

VulkanRaytracerScene::~VulkanRaytracerScene() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
}

VulkanRaytracerScene::Image::Image(const Scene::Image& image, const std::vector<uint8_t>& images_data, const VkDevice device, const VmaAllocator allocator, const std::vector<uint32_t>& queue_family_indices, ComputeHelpers* compute_helpers)
	: Scene::Image(image)
{
	uint32_t w, h, c;
	uint8_t* pixels = stbi_load_from_memory(images_data.data() + image.GetDataOffset(), static_cast<int>(image.GetDataSize()),
		reinterpret_cast<int*>(&w), reinterpret_cast<int*>(&h), reinterpret_cast<int*>(&c), 4);

	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
	if (image.GetName().contains("normal") || image.GetName().contains("NRM") || image.GetName().contains("nrm"))
	{
		format = VK_FORMAT_R8G8B8A8_SNORM;
	}

	mImageResource = std::make_unique<ImageResource>(
		device, VkExtent3D{ w, h, 1 }, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		allocator,
		queue_family_indices,
		"texture"
	);

	auto staging_buffer = std::make_unique<HostBufferResource>(
		device, allocator, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		mImageResource->GetAllocationInfo2().allocationInfo.size, "texture staging"
	);

	std::memcpy(
		staging_buffer->GetAllocationInfo2().allocationInfo.pMappedData,
		pixels,
		w * h * 4
	);

	compute_helpers->RecordBatch();
	compute_helpers->ChangeImageLayout(
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		VK_IMAGE_ASPECT_COLOR_BIT, mImageResource->GetVkImage()
	);
	compute_helpers->CopyBufferToImage(staging_buffer->GetVkBuffer(), mImageResource->GetVkImage(), VkExtent3D{ w,h,1 });
	compute_helpers->SubmitBatch();
}

VulkanRaytracerScene::Image::Image(const char* image_path, const VkDevice device, const VmaAllocator allocator, const std::vector<uint32_t>& queue_family_indices, ComputeHelpers* compute_helpers)
{
	uint32_t w, h, c;
	uint8_t* pixels = stbi_load(image_path, reinterpret_cast<int*>(&w), reinterpret_cast<int*>(&h), reinterpret_cast<int*>(&c), 4);

	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

	mImageResource = std::make_unique<ImageResource>(
		device, VkExtent3D{ w, h, 1 }, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		allocator,
		queue_family_indices,
		"texture"
	);

	auto staging_buffer = std::make_unique<HostBufferResource>(
		device, allocator, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		mImageResource->GetAllocationInfo2().allocationInfo.size, "texture staging"
	);

	std::memcpy(
		staging_buffer->GetAllocationInfo2().allocationInfo.pMappedData,
		pixels,
		w * h * 4
	);

	compute_helpers->RecordBatch();
	compute_helpers->ChangeImageLayout(
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		VK_IMAGE_ASPECT_COLOR_BIT, mImageResource->GetVkImage()
	);
	compute_helpers->CopyBufferToImage(staging_buffer->GetVkBuffer(), mImageResource->GetVkImage(), VkExtent3D{ w,h,1 });
	compute_helpers->SubmitBatch();
}

void VulkanRaytracerScene::Render(const VkCommandBuffer cmd_buff, const DeviceBufferResource* rand_states, const ImageResource* accum_target, const ImageResource* final_render_target, const uint32_t current_sample, const uint32_t width, const uint32_t height, const uint32_t cam_index) const
{
	vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipelineData->GetPipeline());

	const VkDescriptorImageInfo accum_target_desc_info = accum_target->GetDescriptorInfo();
	const VkDescriptorImageInfo final_render_target_desc_info = final_render_target->GetDescriptorInfo();
	const VkDescriptorBufferInfo rand_states_desc_info = rand_states->GetDescriptorInfo();

	const VkWriteDescriptorSet write_desc_sets[] = {
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = mSceneDescSet,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.pImageInfo = &accum_target_desc_info,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = mSceneDescSet,
			.dstBinding = 1,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.pImageInfo = &final_render_target_desc_info,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = mSceneDescSet,
			.dstBinding = 2,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &rand_states_desc_info,
		}
	};

	vkUpdateDescriptorSets(mDevice, std::size(write_desc_sets), write_desc_sets, 0, nullptr);

	const uint32_t offset = static_cast<uint32_t>(mCameraInstances[cam_index].GetViewInverseMatrixOffset());
	const VkBindDescriptorSetsInfoKHR cam_ds_bind_info = {
		.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
		.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		.layout = mPipelineData->GetPipelineLayout(),
		.descriptorSetCount = 1,
		.pDescriptorSets = &mCameraDescSet,
		.dynamicOffsetCount = 1,
		.pDynamicOffsets = &offset,
	};

	vkCmdBindDescriptorSets2KHR(cmd_buff, &cam_ds_bind_info);

	const VkBindDescriptorSetsInfoKHR scene_ds_bind_info = {
		.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
		.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		.layout = mPipelineData->GetPipelineLayout(),
		.firstSet = 1,
		.descriptorSetCount = 1,
		.pDescriptorSets = &mSceneDescSet,
	};

	vkCmdBindDescriptorSets2KHR(cmd_buff, &scene_ds_bind_info);

	const VkPushConstantsInfoKHR pc_info = {
		.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO_KHR,
		.layout = mPipelineData->GetPipelineLayout(),
		.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		.size = sizeof(VulkanRaytracerScenePipelineData::PushConstants),
		.pValues = &current_sample,
	};
	vkCmdPushConstants2KHR(cmd_buff, &pc_info);

	const VkStridedDeviceAddressRegionKHR rg_sbt = {
		.deviceAddress = mRGSbt->GetDeviceAddress(),
		.stride = ALIGNED_SIZE(mRaytracingProperties.shaderGroupHandleSize, mRaytracingProperties.shaderGroupHandleAlignment),
		.size = rg_sbt.stride,
	};

	const VkStridedDeviceAddressRegionKHR ms_sbt = {
		.deviceAddress = mMSSbt->GetDeviceAddress(),
		.stride = ALIGNED_SIZE(mRaytracingProperties.shaderGroupHandleSize, mRaytracingProperties.shaderGroupHandleAlignment),
		.size = ALIGNED_SIZE(mRaytracingProperties.shaderGroupHandleSize, mRaytracingProperties.shaderGroupBaseAlignment),
	};

	const VkStridedDeviceAddressRegionKHR ch_sbt = {
		.deviceAddress = mCHSbt->GetDeviceAddress(),
		.stride = mCHSbtRecordAlignedSize,
		.size = mCHSbtAlignedSize,
	};

	const VkStridedDeviceAddressRegionKHR cl_sbt = {};

	vkCmdTraceRaysKHR(cmd_buff, &rg_sbt, &ms_sbt, &ch_sbt, &cl_sbt, width, height, 1);
}

VkAccelerationStructureKHR VulkanRaytracerScene::GetTLAS() const
{
	return mTLAS->GetAS();
}

VulkanRaytracerScenePipelineData* VulkanRaytracerScene::GetPipelineData() const
{
	return mPipelineData.get();
}
