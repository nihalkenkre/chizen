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
		uint32_t MaterialIndex;
		uint32_t IsCPUShading;
	};

private:
	VkPipeline mPipeline = VK_NULL_HANDLE;
	VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;

	VkDevice mDevice = VK_NULL_HANDLE;
};

ViewportScenePipelineData::ViewportScenePipelineData(const VkDevice device, const size_t num_images, const std::string& name) : mDevice(device)
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

	slang::SessionDesc session_desc = {
		.targets = target_descs,
		.targetCount = std::size(target_descs),
		.compilerOptionEntries = compiler_options,
		.compilerOptionEntryCount = std::size(compiler_options),
	};

	Slang::ComPtr<slang::ISession> compile_session;
	SLANG_CHECK("create compile session", slang_global_session->createSession(session_desc, compile_session.writeRef()));

	const std::string slang_shader_path = std::string(SDL_GetBasePath()).append("/shaders/slang/viewport.slang");

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
	VK_CHECK("create vert shader module", vkCreateShaderModule(mDevice, &vert_mod_ci, nullptr, &vert_mod));

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
	VK_CHECK("create frag shader module", vkCreateShaderModule(mDevice, &frag_mod_ci, nullptr, &frag_mod));

	/*std::filesystem::path vert_path = std::string(current_path).append("/shaders/glsl/viewport.vert.glsl.spv");
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

	std::filesystem::path frag_path = std::string(current_path).append("/shaders/glsl/viewport.frag.glsl.spv");
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
	}*/

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
			.stride = sizeof(float) * 3,
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		},
		{
			.binding = 2,
			.stride = sizeof(float) * 2,
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
			.format = VK_FORMAT_R32G32B32_SFLOAT,
		},
		{
			.location = 2,
			.binding = 2,
			.format = VK_FORMAT_R32G32_SFLOAT,
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
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		}
	};

	const VkDescriptorSetLayoutBinding dsl_1_binds[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		},
	};

	// had to use vector because of num_images (const variable) ?!?!
	const std::vector<VkDescriptorSetLayoutBinding> dsl_2_binds = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = static_cast<uint32_t>(num_images),
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
	};

	const VkDescriptorBindingFlagsEXT binding_flags[] = {
		0, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
	};

	const VkDescriptorSetLayoutBindingFlagsCreateInfo dsl_2_binds_flags_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.bindingCount = std::size(binding_flags),
		.pBindingFlags = binding_flags,
	};

	const VkDescriptorSetLayoutCreateInfo dsl_cis[] = {
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = std::size(dsl_0_binds),
			.pBindings = dsl_0_binds,
		},
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = std::size(dsl_1_binds),
			.pBindings = dsl_1_binds,
		},
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = &dsl_2_binds_flags_ci,
			.bindingCount = static_cast<uint32_t>(std::size(dsl_2_binds)),
			.pBindings = dsl_2_binds.data(),
		}
	};

	mDescriptorSetLayouts.resize(std::size(dsl_cis));

	for (size_t dsl_ci = 0; dsl_ci < std::size(dsl_cis); ++dsl_ci)
	{
		VK_CHECK("create dsl", vkCreateDescriptorSetLayout(mDevice, &dsl_cis[dsl_ci], nullptr, &mDescriptorSetLayouts[dsl_ci]));
	}

	const VkPushConstantRange pc_ranges[] = {
		{
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
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

ViewportWorldScene::ViewportWorldScene(const VulkanScene* scene, const VulkanInterface* vulkan_interface)
	: mDevice(vulkan_interface->GetVkDevice()), mScene(scene)
{
	mPipelineData = std::make_unique<ViewportScenePipelineData>(mDevice, std::max(static_cast<size_t>(1), scene->GetImages().size()), "Scene");

	VmaAllocator allocator = vulkan_interface->GetVmaAllocator();

	const VkDescriptorPoolSize view_proj_desc_size = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		.descriptorCount = 1,
	};

	const VkDescriptorPoolSize model_mat_desc_size = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		.descriptorCount = 1,
	};

	const VkDescriptorPoolSize imgs_desc_pool_size = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = static_cast<uint32_t>(std::max(static_cast<size_t>(1), scene->GetImages().size())),
	};

	const VkDescriptorPoolSize pool_sizes[] = {
		view_proj_desc_size,
		model_mat_desc_size,
		imgs_desc_pool_size,
	};

	const VkDescriptorPoolCreateInfo dsp_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = view_proj_desc_size.descriptorCount + model_mat_desc_size.descriptorCount + 1, //1 for the textures desc set
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes,
	};

	VK_CHECK("create descriptor pool", vkCreateDescriptorPool(mDevice, &dsp_ci, nullptr, &mDescriptorPool));

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_DESCRIPTOR_POOL, reinterpret_cast<uint64_t>(mDescriptorPool), "scene descriptor pool");
#endif // _DEBUG

	const std::vector<VkDescriptorSetLayout>& desc_set_layouts = mPipelineData->GetDescriptorSetLayouts();

	mCameraDescBuffer = {
		.buffer = scene->GetUniformData()->GetVkBuffer(),
		.offset = 0,
		.range = sizeof(glm::mat4),
	};

	const VkDescriptorSetAllocateInfo cam_ds_ai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = mDescriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = desc_set_layouts.data(),
	};
	VK_CHECK("allocate cam matrix desc set", vkAllocateDescriptorSets(mDevice, &cam_ds_ai, &mCameraMatrixDescSet));

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(mCameraMatrixDescSet), "viewport scene cam desc set");
#endif

	const VkWriteDescriptorSet cam_ds_write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = mCameraMatrixDescSet,
		.dstBinding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		.pBufferInfo = &mCameraDescBuffer,
	};

	vkUpdateDescriptorSets(mDevice, 1, &cam_ds_write, 0, nullptr);

	const VkDescriptorSetAllocateInfo model_ds_ai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = mDescriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = desc_set_layouts.data() + 1,
	};
	VK_CHECK("allocate model matrix desc set", vkAllocateDescriptorSets(mDevice, &model_ds_ai, &mModelMatrixDescSet));

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(mModelMatrixDescSet), "viewport scene model matrix desc set");
#endif

	const VkWriteDescriptorSet model_ds_write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = mModelMatrixDescSet,
		.dstBinding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		.pBufferInfo = &mCameraDescBuffer,
	};

	vkUpdateDescriptorSets(mDevice, 1, &model_ds_write, 0, nullptr);

	const uint32_t tex_desc_counts[] = {
		static_cast<uint32_t>(std::max(static_cast<size_t>(1), scene->GetImages().size()))
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
		.pSetLayouts = desc_set_layouts.data() + 2,
	};

	VK_CHECK("create mat tex desc set", vkAllocateDescriptorSets(mDevice, &tex_ds_ai, &mMTexturesDescSet));

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(mMTexturesDescSet), "textures desc set");
#endif // _DEBUG

	const VkDescriptorBufferInfo mat_desc_buff = {
		.buffer = scene->GetMaterialsData()->GetVkBuffer(),
		.range = VK_WHOLE_SIZE,
	};

	std::vector<VkDescriptorImageInfo> image_descs;
	image_descs.reserve(scene->GetImages().size());

	for (const auto& image : scene->GetImages())
	{
		image_descs.push_back(image.GetImageResource()->GetDescriptorInfo());
	}

	const VkWriteDescriptorSet write_descs[] = {
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = mMTexturesDescSet,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pBufferInfo = &mat_desc_buff,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = mMTexturesDescSet,
			.dstBinding = 1,
			.descriptorCount = static_cast<uint32_t>(image_descs.size()),
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = image_descs.data(),
		},
	};

	vkUpdateDescriptorSets(mDevice, std::size(write_descs), write_descs, 0, nullptr);
}

void ViewportWorldScene::Render(const VkCommandBuffer cmd_buff, const uint32_t cam_index, const bool is_cpu_shading) const
{
	vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineData->GetPipeline());

	const uint32_t offset = static_cast<uint32_t>(mScene->GetCameraInstances()[cam_index].GetViewProjMatrixOffset());
	const VkBindDescriptorSetsInfoKHR bind_ds_info = {
		.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.layout = mPipelineData->GetPipelineLayout(),
		.descriptorSetCount = 1,
		.pDescriptorSets = &mCameraMatrixDescSet,
		.dynamicOffsetCount = 1,
		.pDynamicOffsets = &offset,
	};

	vkCmdBindDescriptorSets2KHR(cmd_buff, &bind_ds_info);

	size_t instance_index = 0;
	for (const auto& mesh_instance : mScene->GetMeshInstances())
	{
		const uint32_t offset = static_cast<uint32_t>(mesh_instance.GetModelMatrixOffset());
		const VkBindDescriptorSetsInfoKHR bind_ds_info = {
			.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.layout = mPipelineData->GetPipelineLayout(),
			.firstSet = 1,
			.descriptorSetCount = 1,
			.pDescriptorSets = &mModelMatrixDescSet,
			.dynamicOffsetCount = 1,
			.pDynamicOffsets = &offset,
		};

		vkCmdBindDescriptorSets2KHR(cmd_buff, &bind_ds_info);

		const auto& curr_prims = mScene->GetMeshes()[mesh_instance.GetMeshIndex()].GetPrimitives();

		for (const auto& curr_prim : curr_prims)
		{
			const VkDescriptorSet tex_desc_set = mMTexturesDescSet;
			const VkBindDescriptorSetsInfoKHR bind_ds_info = {
				.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				.layout = mPipelineData->GetPipelineLayout(),
				.firstSet = 2,
				.descriptorSetCount = 1,
				.pDescriptorSets = &tex_desc_set,
			};

			vkCmdBindDescriptorSets2KHR(cmd_buff, &bind_ds_info);

			const ViewportScenePipelineData::PushConstants pc = {
				.MaterialIndex = curr_prim.GetMaterialIndex(),
				.IsCPUShading = is_cpu_shading,
			};

			const VkPushConstantsInfoKHR pc_info = {
				.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO_KHR,
				.layout = mPipelineData->GetPipelineLayout(),
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				.size = sizeof(ViewportScenePipelineData::PushConstants),
				.pValues = &pc
			};
			vkCmdPushConstants2KHR(cmd_buff, &pc_info);

			const VkBuffer buffers[] = {
				mScene->GetVertexData()->GetVkBuffer(),
				mScene->GetVertexData()->GetVkBuffer(),
				mScene->GetVertexData()->GetVkBuffer(),
			};

			const VkDeviceSize offsets[] = {
				curr_prim.GetPositionsOffset(),
				curr_prim.GetNormalsOffset(),
				curr_prim.GetTexcoordsOffset(),
			};

			vkCmdBindVertexBuffers2EXT(cmd_buff, 0, std::size(buffers), buffers, offsets, nullptr, nullptr);

			if (curr_prim.GetIndexCount() != 0)
			{
				vkCmdBindIndexBuffer2KHR(cmd_buff,
					mScene->GetVertexData()->GetVkBuffer(),
					curr_prim.GetIndicesOffset(),
					curr_prim.GetIndicesSize(),
					curr_prim.GetIndexType()
				);

				vkCmdDrawIndexed(cmd_buff, static_cast<uint32_t>(curr_prim.GetIndexCount()), 1, 0, 0, 0);
			}
			else
			{
				vkCmdDraw(cmd_buff, static_cast<uint32_t>(curr_prim.GetVertexCount()), 1, 0, 0);
			}
		}
	}
}

ViewportWorldScene::~ViewportWorldScene() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
	}
}
