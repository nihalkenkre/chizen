#include "app.hpp"
#include "utils.hpp"

App::App(SDL_Window* window, const std::string& current_path)
{
	mVulkanInterface = std::make_unique<VulkanInterface>(window);
	mDisplay = std::make_unique<Display>(mVulkanInterface.get(), current_path);
	mWindow = window;
	VkExtent3D extent = VkExtent3D{ mRenderTargetExtent.width, mRenderTargetExtent.height, 1 };
	mRaytrace = std::make_unique<Raytrace>(mVulkanInterface.get(), extent, current_path);

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

	VK_CHECK("create imgui desc pool", vkCreateDescriptorPool(mVulkanInterface->GetDevice()->GetDevice(), &pool_info, nullptr, &mImGUIPool));

	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	VkFormat color_attachment_format = mVulkanInterface->GetSurface()->GetSurfaceFormat().format;
	ImGui_ImplVulkan_InitInfo imgui_init_info = {
		.Instance = mVulkanInterface->GetInstance()->GetInstance(),
		.PhysicalDevice = mVulkanInterface->GetPhysicalDeviceData()->PhysicalDevice,
		.Device = mVulkanInterface->GetDevice()->GetDevice(),
		.Queue = mVulkanInterface->GetDevice()->GetGraphicsQueue(),
		.DescriptorPool = mImGUIPool,
		.MinImageCount = mVulkanInterface->GetSwapchain()->GetImagesCount(),
		.ImageCount = mVulkanInterface->GetSwapchain()->GetImagesCount(),
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

void App::RunDisplay()
{
	mDisplay->Render(mDeltaMousePosition, mZoomLevel, &mImGUIState);
}

void App::RunRaytrace()
{
	if (!mIsRaytracing)
	{
		mRaytraceThread = std::thread(&Raytrace::Render, mRaytrace.get(), &mIsRaytracing, mMaxSamples);
		mRaytraceThread.detach();
		mIsRaytracing = true;
		mImGUIState.StartRaytracing = false;
	}
}

void App::RecreateRenderTarget()
{
	VK_CHECK("device wait idle", vkDeviceWaitIdle(mVulkanInterface->GetDevice()->GetDevice()));
	mVulkanInterface->RecreateFinalRenderTarget({ mRenderTargetExtent.width, mRenderTargetExtent.height, 1 });
	mDisplay->UpdateFinalRenderTarget(mVulkanInterface->GetFinalRenderTarget());
	mRaytrace->RecreateRenderResources(mRenderTargetExtent);
	mRaytrace->UpdateFinalRenderTarget(mVulkanInterface->GetFinalRenderTarget());
}

void App::StopRaytracing()
{
	mRaytrace->StopRendering();
	while (mIsRaytracing) {}
}

VulkanInterface* App::GetVulkanInterface() const
{
	return mVulkanInterface.get();
}

SDL_Window* App::GetWindow() const
{
	return mWindow;
}

bool& App::IsTrackingMouse()
{
	return mIsTrackingMouse;
}

float* App::GetDeltaMousePosition()
{
	return mDeltaMousePosition;
}

float* App::GetLastMousePosition()
{
	return mLastMousePosition;
}

float& App::GetZoomLevel()
{
	return mZoomLevel;
}

uint32_t& App::GetMaxSamples()
{
	return mMaxSamples;
}

ImGUIState& App::GetImGUIState()
{
	return mImGUIState;
}

bool& App::GetIsRaytracing()
{
	return mIsRaytracing;
}

VkExtent2D& App::GetRenderTargetExtent()
{
	return mRenderTargetExtent;
}

App::~App() noexcept
{
	mRaytrace->StopRendering();

	while (mIsRaytracing) {}

	VK_CHECK("device wait idle", vkDeviceWaitIdle(mVulkanInterface->GetDevice()->GetDevice()));

	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplVulkan_Shutdown();
	ImGui::DestroyContext();

	vkDestroyDescriptorPool(mVulkanInterface->GetDevice()->GetDevice(), mImGUIPool, nullptr);
}
