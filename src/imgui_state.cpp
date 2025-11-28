#include "imgui_state.hpp"
#include "vulkan_interface.hpp"
#include "utils.hpp"
#include "vulkan_objects.hpp"

ImGUIState::ImGUIState(const VulkanInterface* vulkan_interface)
{
	const VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	const	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 1000,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes,
	};

	VK_CHECK("create imgui desc pool", vkCreateDescriptorPool(vulkan_interface->GetDevice()->GetDevice(), &pool_info, nullptr, &mDescriptorPool));

	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	VkFormat color_attachment_format = vulkan_interface->GetSurface()->GetSurfaceFormat().format;
	ImGui_ImplVulkan_InitInfo imgui_init_info = {
		.Instance = vulkan_interface->GetInstance()->GetInstance(),
		.PhysicalDevice = vulkan_interface->GetPhysicalDeviceData()->PhysicalDevice,
		.Device = vulkan_interface->GetDevice()->GetDevice(),
		.Queue = vulkan_interface->GetDevice()->GetGraphicsQueue(),
		.DescriptorPool = mDescriptorPool,
		.MinImageCount = vulkan_interface->GetSwapchain()->GetImagesCount(),
		.ImageCount = vulkan_interface->GetSwapchain()->GetImagesCount(),
		.PipelineInfoMain = {
			.PipelineRenderingCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
				.colorAttachmentCount = 1,
				.pColorAttachmentFormats = &color_attachment_format,
			},
		},
		.UseDynamicRendering = true,
	};

	SDL_CHECK(ImGui_ImplVulkan_Init(&imgui_init_info));
}

ImGUIState::~ImGUIState() noexcept
{
	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplVulkan_Shutdown();
	ImGui::DestroyContext();

	if (mDevice != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
}

bool& ImGUIState::GetStartRaytracing()
{
	return mStartRaytracing;
}

int& ImGUIState::GetMaxSamples()
{
	return mMaxSamples;
}

int* ImGUIState::GetRenderTargetExtent()
{
	return mRenderTargetExtent;
}
