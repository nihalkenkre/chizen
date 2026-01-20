#include "viewport_scene.hpp"
#include "utils.hpp"
#include "resources.hpp"
#include "vulkan_objects.hpp"
#include "vulkan_interface.hpp"
#include "vulkan_scene.hpp"

#include <stb_image.h>

class ViewportScenePipelineData
{
public:
	ViewportScenePipelineData() = delete;
	ViewportScenePipelineData(const VkDevice device, const size_t num_images, const std::string& name);

	ViewportScenePipelineData(const ViewportScenePipelineData& other) = delete;
	ViewportScenePipelineData& operator=(const ViewportScenePipelineData& other) = delete;

	~ViewportScenePipelineData() noexcept;

	VkPipeline GetPipeline() const;
	VkPipelineLayout GetPipelineLayout() const;
	const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const;

	struct PushConstants
	{
		VkDeviceAddress ViewProjMatrix = 0;
		VkDeviceAddress DrawElements = 0;
		VkDeviceAddress Materials = 0;
		VkDeviceAddress ModelMatrices = 0;
		VkDeviceAddress Lights = 0;
		uint32_t LightsCount = 0;
		uint32_t IsCPUShading = 0;
	};

private:
	VkPipeline mPipeline = VK_NULL_HANDLE;
	VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;

	VkDevice mDevice = VK_NULL_HANDLE;
};

ViewportScenePipelineData::ViewportScenePipelineData(const VkDevice device, const size_t num_images, const std::string& name) : mDevice(device)
{
	std::filesystem::path spv_path = std::string(SDL_GetBasePath()).append("shaders\\slang\\viewport.slang.spv");
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
		VK_CHECK("create module", vkCreateShaderModule(mDevice, &ci, nullptr, &mod));
	}
	else
	{
		std::println("Could not find {}", spv_path.string());
	}

	const VkPipelineShaderStageCreateInfo stages[] = {
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = mod,
			.pName = "vertex",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = mod,
			.pName = "fragment",
		},
	};

	const VkVertexInputBindingDescription vibds[] = {
		{
			.binding = 0,
			.stride = sizeof(glm::vec3),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		},
		{
			.binding = 1,
			.stride = sizeof(glm::vec4) + sizeof(glm::vec3) + sizeof(glm::vec2),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		},
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
			.format = VK_FORMAT_R32G32B32A32_SFLOAT,
		},
		{
			.location = 2,
			.binding = 1,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = sizeof(glm::vec4),
		},
		{
			.location = 3,
			.binding = 1,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = sizeof(glm::vec4) + sizeof(glm::vec3),
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
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = static_cast<uint32_t>(num_images),
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
	};

	const VkDescriptorBindingFlagsEXT binding_flags[] = {
		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
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
			.bindingCount = std::size(dsl_0_binds),
			.pBindings = dsl_0_binds,
		}
	};

	mDescriptorSetLayouts.resize(std::size(dsl_cis));

	for (size_t dsl_ci = 0; dsl_ci < std::size(dsl_cis); ++dsl_ci)
	{
		VK_CHECK("create dsl", vkCreateDescriptorSetLayout(mDevice, &dsl_cis[dsl_ci], nullptr, &mDescriptorSetLayouts[dsl_ci]));
	}

	const VkPushConstantRange pc_ranges[] = {
		{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.size = sizeof(ViewportScenePipelineData::PushConstants),
		}
	};

	const VkPipelineLayoutCreateInfo lyt_ci = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = static_cast<uint32_t>(std::size(mDescriptorSetLayouts)),
		.pSetLayouts = mDescriptorSetLayouts.data(),
		.pushConstantRangeCount = std::size(pc_ranges),
		.pPushConstantRanges = pc_ranges,
	};

	VK_CHECK("create graphics pipeline layout", vkCreatePipelineLayout(mDevice, &lyt_ci, nullptr, &mPipelineLayout));

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

	vkDestroyShaderModule(mDevice, mod, nullptr);

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(mPipeline), std::string(name).append(" pipeline").c_str());
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_PIPELINE_LAYOUT, reinterpret_cast<uint64_t>(mPipelineLayout), std::string(name).append(" pipeline layout").c_str());

	for (size_t dsl = 0; dsl < mDescriptorSetLayouts.size(); ++dsl)
	{
		Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, reinterpret_cast<uint64_t>(mDescriptorSetLayouts[dsl]), std::string(name).append(" descriptor set layout ").append(std::to_string(dsl)));
	}
#endif // _DEBUG
}

ViewportScenePipelineData::~ViewportScenePipelineData() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
	{
		for (auto& desc_set_layout : mDescriptorSetLayouts)
			vkDestroyDescriptorSetLayout(mDevice, desc_set_layout, nullptr);

		vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
		vkDestroyPipeline(mDevice, mPipeline, nullptr);
	}
}

VkPipeline ViewportScenePipelineData::GetPipeline() const
{
	return mPipeline;
}

VkPipelineLayout ViewportScenePipelineData::GetPipelineLayout() const
{
	return mPipelineLayout;
}

const std::vector<VkDescriptorSetLayout>& ViewportScenePipelineData::GetDescriptorSetLayouts() const
{
	return mDescriptorSetLayouts;
}

ViewportWorldScene::ViewportWorldScene(const VulkanScene* vulkan_scene, const VulkanInterface* vulkan_interface)
	: mDevice(vulkan_interface->GetVkDevice()), mAllocator(vulkan_interface->GetVmaAllocator()), mScene(vulkan_scene)
{
	mPipelineData = std::make_unique<ViewportScenePipelineData>(mDevice, std::max(static_cast<size_t>(1), vulkan_scene->GetImages().size()), "Scene");

	CreateIndirectBufferAndDrawElementsBuffer(vulkan_interface->GetTransferHelpers());
	CreateDescriptorPool();
	CreateDescriptorSets();
}

void ViewportWorldScene::Render(const VkCommandBuffer cmd_buff, const uint32_t camera_index, const bool is_cpu_shading) const
{
	vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineData->GetPipeline());


	const VkDescriptorSet desc_sets[] = {
		mMTexturesDescSet,
	};

	const VkBindDescriptorSetsInfoKHR bind_ds_info = {
		.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.layout = mPipelineData->GetPipelineLayout(),
		.firstSet = 0,
		.descriptorSetCount = _countof(desc_sets),
		.pDescriptorSets = desc_sets,
	};

	vkCmdBindDescriptorSets2(cmd_buff, &bind_ds_info);

	const VkBuffer bindings[] = {
		mScene->GetPositionsData()->GetVkBuffer(),
		mScene->GetVertexData()->GetVkBuffer(),
	};

	const VkDeviceSize bindings_offsets[] = {
		0,
		0,
	};

	vkCmdBindVertexBuffers2(cmd_buff, 0, std::size(bindings), bindings, bindings_offsets, nullptr, nullptr);
	vkCmdBindIndexBuffer2(cmd_buff, mScene->GetIndexData()->GetVkBuffer(), 0, mScene->GetIndicesSize(), VK_INDEX_TYPE_UINT32);

	const ViewportScenePipelineData::PushConstants pcf = {
		.ViewProjMatrix = mScene->GetCameraMatricesData()->GetDeviceAddress() + (camera_index * 3 * sizeof(glm::mat4)),
		.DrawElements = mDrawElements->GetDeviceAddress(),
		.Materials = mScene->GetMaterialsData()->GetDeviceAddress(),
		.ModelMatrices = mScene->GetModelMatricesData()->GetDeviceAddress(),
		.Lights = mScene->GetLightsData()->GetDeviceAddress(),
		.LightsCount = static_cast<uint32_t>(mScene->GetLights().size()),
		.IsCPUShading = is_cpu_shading,
	};

	const VkPushConstantsInfoKHR pc_info = {
		.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO_KHR,
		.layout = mPipelineData->GetPipelineLayout(),
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		.size = sizeof(ViewportScenePipelineData::PushConstants),
		.pValues = &pcf,
	};
	vkCmdPushConstants2(cmd_buff, &pc_info);

	vkCmdDrawIndexedIndirect(cmd_buff, mIndirectCommandsBuffer->GetVkBuffer(), 0, mDrawElementsCount, sizeof(VkDrawIndexedIndirectCommand));
}

void ViewportWorldScene::ReloadShaders()
{
	mPipelineData.reset();
	mPipelineData = std::make_unique<ViewportScenePipelineData>(mDevice, std::max(static_cast<size_t>(1), mScene->GetImages().size()), "Scene");

	if (mDescriptorPool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);

	CreateDescriptorPool();
	CreateDescriptorSets();
}

void ViewportWorldScene::CreateIndirectBufferAndDrawElementsBuffer(TransferHelpers* transfer_helpers)
{
	struct DrawElement
	{
		uint32_t model_matrix_index = 0;
		uint32_t material_index = 0;
	};

	std::vector<VkDrawIndexedIndirectCommand> indirect_commands;
	std::vector<DrawElement> draw_elements;

	for (const auto& mesh_instance : mScene->GetMeshInstances())
	{
		auto curr_mesh = mScene->GetMeshes()[mesh_instance.GetMeshIndex()];

		for (const auto& curr_prim : curr_mesh.GetPrimitives())
		{
			draw_elements.push_back(
				DrawElement{
					.model_matrix_index = static_cast<uint32_t>(mesh_instance.GetModelMatrixOffset() / 64), // just store the index in the scene mesh instance object. :|
					.material_index = static_cast<uint32_t>(curr_prim.GetMaterialIndex()),
				}
				);

			indirect_commands.push_back(
				VkDrawIndexedIndirectCommand{
					.indexCount = static_cast<uint32_t>(curr_prim.GetIndexCount()),
					.instanceCount = 1,
					.firstIndex = static_cast<uint32_t>(curr_prim.GetFirstIndexIndex()),
				}
				);

			++mDrawElementsCount;
		}
	}

	std::vector<uint8_t> indirect_commands_data(indirect_commands.size() * sizeof(VkDrawIndexedIndirectCommand));
	std::memcpy(
		indirect_commands_data.data(),
		indirect_commands.data(),
		indirect_commands_data.size()
	);

	auto staging_indirect_commands = std::make_unique<HostBufferResource>(
		mDevice, mAllocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		indirect_commands_data, "staging indirect commands"
	);
	mIndirectCommandsBuffer = std::make_unique<DeviceBufferResource>(
		mDevice, mAllocator, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		indirect_commands_data.size(),
		"indirect commands"
	);

	std::vector<uint8_t> draw_elements_data(draw_elements.size() * sizeof(DrawElement));
	std::memcpy(
		draw_elements_data.data(),
		draw_elements.data(),
		draw_elements_data.size()
	);

	auto staging_draw_elements = std::make_unique<HostBufferResource>(
		mDevice, mAllocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		draw_elements_data, "staging draw elements"
	);
	mDrawElements = std::make_unique<DeviceBufferResource>(
		mDevice, mAllocator, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		draw_elements_data.size(), "draw elements"
	);

	transfer_helpers->RecordBatch();
	transfer_helpers->CopyBufferToBuffer(staging_indirect_commands->GetVkBuffer(), mIndirectCommandsBuffer->GetVkBuffer(), indirect_commands_data.size());
	transfer_helpers->CopyBufferToBuffer(staging_draw_elements->GetVkBuffer(), mDrawElements->GetVkBuffer(), draw_elements_data.size());
	transfer_helpers->SubmitBatch();
}

void ViewportWorldScene::CreateDescriptorPool()
{
	const VkDescriptorPoolSize imgs_desc_pool_size = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = static_cast<uint32_t>(std::max(static_cast<size_t>(1), mScene->GetImages().size())),
	};

	const VkDescriptorPoolSize pool_sizes[] = {
		imgs_desc_pool_size,
	};

	const VkDescriptorPoolCreateInfo dsp_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1, //1 for the textures desc set
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes,
	};

	VK_CHECK("create descriptor pool", vkCreateDescriptorPool(mDevice, &dsp_ci, nullptr, &mDescriptorPool));

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_DESCRIPTOR_POOL, reinterpret_cast<uint64_t>(mDescriptorPool), "scene descriptor pool");
#endif // _DEBUG
}

void ViewportWorldScene::CreateDescriptorSets()
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

	const VkDescriptorSetAllocateInfo tex_ds_ai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = &tex_ds_vdcai,
		.descriptorPool = mDescriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = desc_set_layouts.data(),
	};

	VK_CHECK("create mat tex desc set", vkAllocateDescriptorSets(mDevice, &tex_ds_ai, &mMTexturesDescSet));

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(mMTexturesDescSet), "textures desc set");
#endif // _DEBUG

	const VkWriteDescriptorSet write_descs[] = {
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = mMTexturesDescSet,
			.dstBinding = 0,
			.descriptorCount = static_cast<uint32_t>(mScene->GetImageDescs().size()),
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = mScene->GetImageDescs().data(),
		},
	};

	vkUpdateDescriptorSets(mDevice, std::size(write_descs), write_descs, 0, nullptr);
}

ViewportWorldScene::~ViewportWorldScene() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
	}
}
