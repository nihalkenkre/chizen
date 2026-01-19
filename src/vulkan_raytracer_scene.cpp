#include "vulkan_raytracer_scene.hpp"
#include "vulkan_objects.hpp"
#include "resources.hpp"
#include "vulkan_interface.hpp"
#include "vulkan_scene.hpp"
#include "utils.hpp"

#include <stb_image.h>

class VulkanRaytracerScenePipelineData
{
public:
	VulkanRaytracerScenePipelineData() = delete;
	VulkanRaytracerScenePipelineData(const VkDevice device, const size_t num_images, const std::string& name);

	VulkanRaytracerScenePipelineData(const VulkanRaytracerScenePipelineData& other) = delete;
	VulkanRaytracerScenePipelineData& operator=(const VulkanRaytracerScenePipelineData& other) = delete;

	~VulkanRaytracerScenePipelineData() noexcept;

	struct PushConstants
	{
		VkDeviceAddress RandomStates = 0;
		VkDeviceAddress CameraInfo = 0;
		VkDeviceAddress Lights = 0;
		VkDeviceAddress Materials = 0;
		uint32_t LightsCount = 0;
		uint32_t CurrentSample = 1;
		uint32_t IsCPUShading = 0;
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

VulkanRaytracerScenePipelineData::VulkanRaytracerScenePipelineData(const VkDevice device, const size_t num_images, const std::string& name)
	: mDevice(device)
{
	std::filesystem::path spv_path = std::string(SDL_GetBasePath()).append("shaders\\slang\\raytrace.slang.spv");
	VkShaderModule mod = VK_NULL_HANDLE;
	if (std::filesystem::exists(spv_path))
	{
		std::uintmax_t spv_size = std::filesystem::file_size(spv_path);
		std::ifstream spv_file(spv_path.c_str(), std::ios::binary);

		std::vector<char> spv_code(spv_size, 0);
		spv_file.read(spv_code.data(), spv_size);

		const VkShaderModuleCreateInfo ci = {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = spv_size,
			.pCode = reinterpret_cast<uint32_t*>(spv_code.data()),
		};
		VK_CHECK("create shader module", vkCreateShaderModule(mDevice, &ci, nullptr, &mod));
	}
	else
	{
		std::println("Could not find {}", spv_path.string());
	}

	const VkDescriptorSetLayoutBinding dsl_0_bindings[] = {
		// Accum Target
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		},
		// Final Target
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		},
		// TLAS
		{
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		},
		// Textures
		{
			.binding = 3,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = static_cast<uint32_t>(num_images),
			.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
		},
	};

	const VkDescriptorBindingFlagsEXT binding_flags[] = {
		0, 0, 0, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT
	};

	const VkDescriptorSetLayoutBindingFlagsCreateInfo dsl_0_binds_flags_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.bindingCount = std::size(binding_flags),
		.pBindingFlags = binding_flags,
	};

	const VkDescriptorSetLayoutCreateInfo dsl_cis[] = {
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = &dsl_0_binds_flags_ci,
			.bindingCount = std::size(dsl_0_bindings),
			.pBindings = dsl_0_bindings,
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
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
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
			.module = mod,
			.pName = "raygen",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_MISS_BIT_KHR,
			.module = mod,
			.pName = "miss",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
			.module = mod,
			.pName = "closesthit",
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

	vkDestroyShaderModule(device, mod, nullptr);

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

VulkanRaytracerScene::VulkanRaytracerScene(const Scene& scene, const VulkanScene* vulkan_scene, const VulkanInterface* vulkan_interface)
	: mDevice(vulkan_interface->GetVkDevice()), mRaytracingProperties(vulkan_interface->GetPhysicalDeviceData()->RayTracingProperties), mScene(vulkan_scene),
	mImageDescs(vulkan_scene->GetImageDescs())
{
	VmaAllocator allocator = vulkan_interface->GetVmaAllocator();
	uint32_t mem_type_id = Utils_GetMemoryTypeId(vulkan_interface->GetPhysicalDeviceData()->MemoryProperties, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VkDeviceSize scratch_buffer_alignment = vulkan_interface->GetPhysicalDeviceData()->AccelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment;
	ComputeHelpers* compute_helpers = vulkan_interface->GetComputeHelpers();

	mPipelineData = std::make_unique<VulkanRaytracerScenePipelineData>(mDevice, std::max(static_cast<size_t>(1), vulkan_scene->GetImages().size()), "vulkan raytracer");
	mScratchBufferPool = std::make_unique<Pool>(allocator, scratch_buffer_alignment, mem_type_id);
	mSBTBufferPool = std::make_unique<Pool>(allocator, mRaytracingProperties.shaderGroupBaseAlignment, mem_type_id);

	const VkDeviceSize sbt_handle_size = mRaytracingProperties.shaderGroupHandleSize;
	const VkDeviceSize sbt_handle_aligned_size = ALIGNED_SIZE(mRaytracingProperties.shaderGroupHandleSize, mRaytracingProperties.shaderGroupHandleAlignment);
	const size_t group_count = mPipelineData->GetShaderGroups().size();
	const size_t sbt_size = group_count * sbt_handle_size;

	std::vector<uint8_t> sbt_handles(sbt_size);
	VK_CHECK("get sbt group handles", vkGetRayTracingShaderGroupHandlesKHR(mDevice, mPipelineData->GetPipeline(), 0, static_cast<uint32_t>(group_count), sbt_size, sbt_handles.data()));

	auto staging_rg_sbt = std::make_unique<HostBufferResource>(
		mDevice, allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		ALIGNED_SIZE(sbt_handle_aligned_size, mRaytracingProperties.shaderGroupBaseAlignment), "staging rg sbt"
	);
	std::memcpy(staging_rg_sbt->GetAllocationInfo2().allocationInfo.pMappedData, sbt_handles.data(), sbt_handle_size);

	mRGSbt = std::make_unique<DeviceBufferResource>(
		mDevice, allocator, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		ALIGNED_SIZE(sbt_handle_aligned_size, mRaytracingProperties.shaderGroupBaseAlignment), "rg sbt", mSBTBufferPool->GetPool()
	);

	auto staging_ms_sbt = std::make_unique<HostBufferResource>(
		mDevice, allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		ALIGNED_SIZE(sbt_handle_aligned_size, mRaytracingProperties.shaderGroupBaseAlignment), "staging ms sbt"
	);
	std::memcpy(staging_ms_sbt->GetAllocationInfo2().allocationInfo.pMappedData, sbt_handles.data() + sbt_handle_aligned_size, sbt_handle_size);

	mMSSbt = std::make_unique<DeviceBufferResource>(
		mDevice, allocator, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		ALIGNED_SIZE(sbt_handle_aligned_size, mRaytracingProperties.shaderGroupBaseAlignment), "ms sbt", mSBTBufferPool->GetPool()
	);

	struct CHSbtRecordData
	{
		VkDeviceAddress vertices_data;
		VkDeviceAddress indices;
		uint64_t MaterialIndex;
	};

	std::vector<uint8_t> ch_sbt;
	mCHSbtRecordAlignedSize = ALIGNED_SIZE(mRaytracingProperties.shaderGroupHandleSize + sizeof(CHSbtRecordData), mRaytracingProperties.shaderGroupHandleAlignment);

	std::vector<VkAccelerationStructureInstanceKHR> instances;
	instances.reserve(vulkan_scene->GetMeshInstances().size());

	// outside vector is meshes, inside vector is offset of prims of meshes
	std::vector<std::vector<uint32_t>> prim_sbt_record_offsets;

	uint32_t total_records = 0;
	for (const auto& mesh : vulkan_scene->GetMeshes())
	{
		std::vector<uint32_t> mesh_prim_offsets;

		for (const auto& prim : mesh.GetPrimitives())
		{
			const CHSbtRecordData ch_sbt_record = {
				.vertices_data = vulkan_scene->GetVertexData()->GetDeviceOrHostAddressConstKHR().deviceAddress,
				.indices = vulkan_scene->GetIndexData()->GetDeviceOrHostAddressConstKHR().deviceAddress + prim.GetIndicesOffset(),
				.MaterialIndex = static_cast<uint64_t>(prim.GetMaterialIndex()),
			};

			std::vector<uint8_t> ch_sbt_record_data(mCHSbtRecordAlignedSize);
			std::memcpy(ch_sbt_record_data.data(), sbt_handles.data() + (sbt_handle_size * 2), sbt_handle_size);
			std::memcpy(ch_sbt_record_data.data() + sbt_handle_size, &ch_sbt_record, sizeof(CHSbtRecordData));

			ch_sbt.append_range(ch_sbt_record_data);

			mesh_prim_offsets.push_back(total_records++);
		}

		prim_sbt_record_offsets.push_back(mesh_prim_offsets);
	}

	for (const auto& mesh_instance : vulkan_scene->GetMeshInstances())
	{
		auto mesh = vulkan_scene->GetMeshes()[mesh_instance.GetMeshIndex()];

		size_t curr_prim_idx = 0;
		for (const auto& prim : mesh.GetPrimitives())
		{
			const VkDeviceOrHostAddressConstKHR positions_addr = {
				.deviceAddress = vulkan_scene->GetPositionsData()->GetDeviceAddress(),
			};

			const VkDeviceOrHostAddressConstKHR indices_addr = {
				.deviceAddress = vulkan_scene->GetIndexData()->GetDeviceAddress() + prim.GetIndicesOffset(),
			};

			mBLASes.push_back(
				std::make_unique<BLAccelerationStructure>(
					mDevice, allocator, prim, positions_addr, indices_addr, mScratchBufferPool->GetPool(), scratch_buffer_alignment, compute_helpers, "BLAS"
				)
			);

			const VkAccelerationStructureDeviceAddressInfoKHR blas_addr_info = {
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
				.accelerationStructure = mBLASes[mBLASes.size() - 1]->GetAS(),
			};

			glm::mat4 xform = {};
			std::memcpy(&xform, scene.GetModelMatricesData().data() + mesh_instance.GetModelMatrixOffset(), sizeof(xform));

			xform = glm::transpose(xform);
			VkTransformMatrixKHR inst_xform;
			std::memcpy(&inst_xform, &xform, sizeof(inst_xform));

			const VkAccelerationStructureInstanceKHR inst = {
				.transform = inst_xform,
				.mask = 0xFF,
				.instanceShaderBindingTableRecordOffset = prim_sbt_record_offsets[mesh_instance.GetMeshIndex()][curr_prim_idx++],
				.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(mDevice, &blas_addr_info),
			};

			instances.push_back(inst);
		}
	}

	mCHSbtAlignedSize = ALIGNED_SIZE(ch_sbt.size(), mRaytracingProperties.shaderGroupBaseAlignment);

	auto staging_ch_sbt = std::make_unique<HostBufferResource>(
		mDevice, allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		mCHSbtAlignedSize, "staging ch sbt"
	);
	std::memcpy(staging_ch_sbt->GetAllocationInfo2().allocationInfo.pMappedData, ch_sbt.data(), ch_sbt.size());

	mCHSbt = std::make_unique<DeviceBufferResource>(
		mDevice, allocator, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		mCHSbtAlignedSize, "ch sbt", mSBTBufferPool->GetPool()
	);

	mTLAS = std::make_unique<TLAccelerationStructure>(
		mDevice, allocator, instances, mScratchBufferPool->GetPool(), scratch_buffer_alignment, compute_helpers, "TLAS"
	);

	CreateDescriptorPool();
	CreateDescriptorSets();

	compute_helpers->RecordBatch();
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

void VulkanRaytracerScene::Render(const VkCommandBuffer cmd_buff, const DeviceBufferResource* rand_states, const ImageResource* accum_target, const ImageResource* final_render_target, const uint32_t current_sample, const uint32_t width, const uint32_t height, const uint32_t camera_index, const bool is_cpu_shading) const
{
	vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, mPipelineData->GetPipeline());

	const VkDescriptorImageInfo accum_target_desc_info = accum_target->GetDescriptorInfo();
	const VkDescriptorImageInfo final_render_target_desc_info = final_render_target->GetDescriptorInfo();

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
	};

	vkUpdateDescriptorSets(mDevice, std::size(write_desc_sets), write_desc_sets, 0, nullptr);

	const VkBindDescriptorSetsInfoKHR scene_ds_bind_info = {
		.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
		.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		.layout = mPipelineData->GetPipelineLayout(),
		.firstSet = 0,
		.descriptorSetCount = 1,
		.pDescriptorSets = &mSceneDescSet,
	};

	vkCmdBindDescriptorSets2(cmd_buff, &scene_ds_bind_info);

	const VulkanRaytracerScenePipelineData::PushConstants pc = {
		.RandomStates = rand_states->GetDeviceAddress(),
		.CameraInfo = mScene->GetCameraMatricesData()->GetDeviceAddress() + ((camera_index * 3 + 1) * sizeof(glm::mat4)),
		.Lights = mScene->GetLightsData()->GetDeviceAddress(),
		.Materials = mScene->GetMaterialsData()->GetDeviceAddress(),
		.LightsCount = static_cast<uint32_t>(mScene->GetLights().size()),
		.CurrentSample = current_sample,
		.IsCPUShading = is_cpu_shading,
	};

	const VkPushConstantsInfoKHR pc_info = {
		.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO_KHR,
		.layout = mPipelineData->GetPipelineLayout(),
		.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
		.size = sizeof(VulkanRaytracerScenePipelineData::PushConstants),
		.pValues = &pc,
	};

	vkCmdPushConstants2(cmd_buff, &pc_info);

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

void VulkanRaytracerScene::ReloadShaders()
{
	mPipelineData.reset();
	mPipelineData = std::make_unique<VulkanRaytracerScenePipelineData>(mDevice, mScene->GetImages().size(), "vulkan raytracer scene");

	if (mDevice != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);

	CreateDescriptorPool();
	CreateDescriptorSets();
}

VkAccelerationStructureKHR VulkanRaytracerScene::GetTLAS() const
{
	return mTLAS->GetAS();
}

VulkanRaytracerScenePipelineData* VulkanRaytracerScene::GetPipelineData() const
{
	return mPipelineData.get();
}

void VulkanRaytracerScene::CreateDescriptorPool()
{
	const VkDescriptorPoolSize pool_sizes[] = {
		// accum target and final target
		{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 2,
		},
		// TLAS
		{
			.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
			.descriptorCount = 1,
		},
		// Textures
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = static_cast<uint32_t>(std::max(static_cast<size_t>(1), mScene->GetImageDescs().size())),
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

}

void VulkanRaytracerScene::CreateDescriptorSets()
{
	const std::vector<VkDescriptorSetLayout>& desc_set_layouts = mPipelineData->GetDescriptorSetLayouts();

	const uint32_t tex_desc_counts[] = {
		static_cast<uint32_t>(std::max(static_cast<size_t>(1), mScene->GetImages().size()))
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
		.pSetLayouts = desc_set_layouts.data(),
	};

	VK_CHECK("allocate desc set", vkAllocateDescriptorSets(mDevice, &ds_ai, &mSceneDescSet));

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(mSceneDescSet), "vulkan raytracer scene descriptor set");
#endif // _DEBUG

	VkAccelerationStructureKHR tlas = mTLAS->GetAS();
	const VkWriteDescriptorSetAccelerationStructureKHR tlas_desc_info = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
		.accelerationStructureCount = 1,
		.pAccelerationStructures = &tlas,
	};

	const VkWriteDescriptorSet ds_writes[] = {
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = &tlas_desc_info,
			.dstSet = mSceneDescSet,
			.dstBinding = 2,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = mSceneDescSet,
			.dstBinding = 3,
			.descriptorCount = static_cast<uint32_t>(mImageDescs.size()),
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = mImageDescs.data(),
		},
	};

	vkUpdateDescriptorSets(mDevice, std::size(ds_writes), ds_writes, 0, nullptr);
}
