#include "app.hpp"
#include "vulkan_interface.hpp"
#include "display.hpp"
#include "raytrace.hpp"
#include "imgui_state.hpp"
#include "utils.hpp"
#include "vulkan_objects.hpp"
#include "resources.hpp"

App::App(SDL_Window* window, const std::string& current_path)
{
	mVulkanInterface = std::make_unique<VulkanInterface>(window);
	mImGUIState = std::make_unique<ImGUIState>(mVulkanInterface.get());

	VkExtent3D extent = VkExtent3D{ mRenderTargetExtent.width, mRenderTargetExtent.height, 1 };
	std::vector<uint32_t> queue_family_indices{ mVulkanInterface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->TransferQueueFamilyIndex};
	mFinalRenderTarget = std::make_unique<ImageResource>(mVulkanInterface->GetDevice()->GetDevice(),
		extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		mVulkanInterface->GetAllocator()->GetAllocator(), 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		queue_family_indices,
		"final render target");

	Utils_InitializeImages({ mFinalRenderTarget->GetImage() }, mVulkanInterface->GetTransferObjects()->GetCommandBuffer(), mVulkanInterface->GetTransferObjects()->GetQueue());

	mDisplay = std::make_unique<Display>(mVulkanInterface.get(), mFinalRenderTarget.get(), current_path);
	mRaytrace = std::make_unique<Raytrace>(mVulkanInterface.get(), mFinalRenderTarget.get(), extent, current_path);
	mWindow = window;
}

void App::RunDisplay()
{
	mDisplay->Render(mDeltaMousePosition, mZoomLevel, mImGUIState.get());
}

void App::RunRaytrace()
{
	if (!mIsRaytracing)
	{
		mRaytraceThread = std::thread(&Raytrace::Render, mRaytrace.get(), &mIsRaytracing, mMaxSamples);
		mRaytraceThread.detach();
		mIsRaytracing = true;
		mImGUIState->GetStartRaytracing() = false;
	}
}

void App::RecreateRenderTarget()
{
	VK_CHECK("device wait idle", vkDeviceWaitIdle(mVulkanInterface->GetDevice()->GetDevice()));

	VkExtent3D extent = { mRenderTargetExtent.width, mRenderTargetExtent.height, 1 };
	std::vector<uint32_t> queue_family_indices{ mVulkanInterface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex,  mVulkanInterface->GetPhysicalDeviceData()->TransferQueueFamilyIndex};
	mFinalRenderTarget = std::make_unique<ImageResource>(mVulkanInterface->GetDevice()->GetDevice(),
		extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		mVulkanInterface->GetAllocator()->GetAllocator(), 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		queue_family_indices,
		"final render target");

	Utils_InitializeImages({ mFinalRenderTarget->GetImage() }, mVulkanInterface->GetTransferObjects()->GetCommandBuffer(), mVulkanInterface->GetTransferObjects()->GetQueue());

	mDisplay->UpdateFinalRenderTarget(mFinalRenderTarget.get());
	mRaytrace->RecreateRenderResources(mRenderTargetExtent);
	mRaytrace->UpdateFinalRenderTarget(mFinalRenderTarget.get());
}

void App::StopRaytracing()
{
	mRaytrace->StopRendering();
	while (mIsRaytracing) {}
}

void App::RecreateSwapchain()
{
	mVulkanInterface->RecreateSwapchain();
	mDisplay->UpdateSwapchain(mVulkanInterface->GetSwapchain());
	mDisplay->UpdateExtent(mVulkanInterface->GetSurface()->GetSurfaceCapabilities().surfaceCapabilities.currentExtent);
}

VulkanInterface* App::GetVulkanInterface() const
{
	return mVulkanInterface.get();
}

Display* App::GetDisplay() const
{
	return mDisplay.get();
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

ImGUIState* App::GetImGUIState()
{
	return mImGUIState.get();
}

uint32_t& App::GetMaxSamples()
{
	return mMaxSamples;
}

bool& App::IsRaytracing()
{
	return mIsRaytracing;
}

VkExtent2D& App::GetRenderTargetExtent()
{
	return mRenderTargetExtent;
}

ImageResource* App::GetFinalRenderTarget() const
{
	return mFinalRenderTarget.get();
}

App::~App() noexcept
{
	mRaytrace->StopRendering();

	while (mIsRaytracing) {}

	VK_CHECK("device wait idle", vkDeviceWaitIdle(mVulkanInterface->GetDevice()->GetDevice()));
}
