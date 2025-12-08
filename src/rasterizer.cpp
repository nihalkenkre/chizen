#include "rasterizer.hpp"
#include "vulkan_interface.hpp"
#include "vulkan_objects.hpp"
#include "frame_objects.hpp"
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
	std::vector<VkDescriptorSetLayout> GetDescriptorSetLayouts() const;

private:
	VkPipeline mPipeline = VK_NULL_HANDLE;
	VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;

	VkDevice mDevice = VK_NULL_HANDLE;
};

RasterizerPipelineData::RasterizerPipelineData(const VulkanInterface* vulkan_interface, const std::string& current_path, const std::string& name) : mDevice(vulkan_interface->GetDevice()->GetDevice())
{
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

	const std::string slang_shader_path = std::string(current_path).append("/shaders/slang/rasterize.slang");

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
	VK_CHECK("create vert shader module", vkCreateShaderModule(vulkan_interface->GetDevice()->GetDevice(), &vert_mod_ci, nullptr, &vert_mod));

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
	VK_CHECK("create frag shader module", vkCreateShaderModule(vulkan_interface->GetDevice()->GetDevice(), &frag_mod_ci, nullptr, &frag_mod));


	vkDestroyShaderModule(mDevice, vert_mod, nullptr);
	vkDestroyShaderModule(mDevice, frag_mod, nullptr);
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

Rasterizer::Rasterizer(const VulkanInterface* vulkan_interface, const std::string& name) : mDevice(vulkan_interface->GetDevice()->GetDevice()), mSwapchain(vulkan_interface->GetSwapchain()), mQueue(vulkan_interface->GetDevice()->GetGraphicsQueue())
{
	mMaxFramesInFlight = static_cast<uint8_t>(vulkan_interface->GetSwapchain()->GetImagesCount());
	mFrameObjects = std::make_unique<FrameObjects>(vulkan_interface->GetDevice()->GetDevice(), vulkan_interface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex, mMaxFramesInFlight, "rasterizer frame objects");

	mAcquireSignalSemaphores.resize(mMaxFramesInFlight, VK_NULL_HANDLE);
	mPresentWaitSemaphores.resize(mMaxFramesInFlight, VK_NULL_HANDLE);
	mDescriptorSets.resize(mMaxFramesInFlight, VK_NULL_HANDLE);

	const VkSemaphoreCreateInfo bin_sem_ci = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	const VkDescriptorPoolSize pool_sizes[] = {
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
		},
	};

	const VkDescriptorPoolCreateInfo dp_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = mMaxFramesInFlight,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes,
	};

	VK_CHECK("create dsp", vkCreateDescriptorPool(vulkan_interface->GetDevice()->GetDevice(), &dp_ci, nullptr, &mDescriptorPool));

	//auto dsls = mPipelineData->GetDescriptorSetLayouts();

	//const VkDescriptorSetAllocateInfo ds_ai = {
	//	.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
	//	.descriptorPool = mDescriptorPool,
	//	.descriptorSetCount = 1,
	//	.pSetLayouts = dsls.data(),
	//};

	for (uint8_t fr = 0; fr < mMaxFramesInFlight; ++fr)
	{
		VK_CHECK("create acq sig semaphore", vkCreateSemaphore(mDevice, &bin_sem_ci, nullptr, mAcquireSignalSemaphores.data() + fr));
		VK_CHECK("create present wait semaphore", vkCreateSemaphore(mDevice, &bin_sem_ci, nullptr, mPresentWaitSemaphores.data() + fr));
		//VK_CHECK("allocate display desc sets", vkAllocateDescriptorSets(mDevice, &ds_ai, mDescriptorSets.data() + fr));
	}
}

void Rasterizer::Render()
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
		mSwapchain->GetImages()[img_idx]);

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

	const VkRenderingInfo rendering_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = {
			.extent = mExtent,
		},
		.layerCount = 1,
		.colorAttachmentCount = std::size(col_attachs),
		.pColorAttachments = col_attachs,
	};

	vkCmdBeginRenderingKHR(cmd_buff, &rendering_info);
	vkCmdEndRenderingKHR(cmd_buff);

	Utils_ChangeImageLayout(cmd_buff,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		mSwapchain->GetImages()[img_idx]);

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
	//VK_CHECK("gfx q wait idle", vkQueueWaitIdle(mQueue));

	mFrameObjects->NextFrame();
}

void Rasterizer::UpdateSwapchain(Swapchain* swapchain)
{
	mSwapchain = swapchain;
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
