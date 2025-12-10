#include "app.hpp"
#include "vulkan_interface.hpp"
#include "rasterizer.hpp"
#include "display.hpp"
#include "raytracer.hpp"
#include "imgui_state.hpp"
#include "utils.hpp"
#include "vulkan_objects.hpp"
#include "resources.hpp"
#include "scene.hpp"
#include "events.hpp"

App::App(SDL_Window* window, const std::string& current_path) : mWindow(window)
{
	mVulkanInterface = std::make_unique<VulkanInterface>(window);
	mImGUIState = std::make_unique<ImGUIState>(mVulkanInterface.get());
	mScene = std::make_unique<EmptyScene>();

	VkExtent3D extent = VkExtent3D{ mRenderTargetExtent.width, mRenderTargetExtent.height, 1 };
	std::vector<uint32_t> queue_family_indices{ mVulkanInterface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->TransferQueueFamilyIndex };
	mFinalRenderTarget = std::make_unique<ImageResource>(mVulkanInterface->GetDevice()->GetDevice(),
		extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		mVulkanInterface->GetAllocator()->GetAllocator(),
		queue_family_indices,
		"final render target");

	Utils_InitializeImages({ mFinalRenderTarget->GetImage() }, mVulkanInterface->GetTransferObjects()->GetCommandBuffer(), mVulkanInterface->GetTransferObjects()->GetQueue());

	mRasterizer = std::make_unique<Rasterizer>(mVulkanInterface.get(), current_path, "rasterizer");
	mDisplay = std::make_unique<Display>(mVulkanInterface.get(), mFinalRenderTarget.get(), current_path);
	mRaytracer = std::make_unique<Raytracer>(mVulkanInterface.get(), mFinalRenderTarget.get(), extent, current_path, "raytrace");
}

void App::ProcessEvent(SDL_Event* event)
{
	if (event->type == events.FileOpen.type)
	{
		std::println("open file {}", reinterpret_cast<const char*>(event->user.data1));
		mScene = std::make_unique<WorldScene>(mVulkanInterface.get(),
			mVulkanInterface->GetTransferObjects()->GetCommandBuffer(),
			mRasterizer->GetDescriptorSetLayouts(),
			mVulkanInterface->GetTransferObjects()->GetQueue(),
			reinterpret_cast<const char*>(event->user.data1)
		);

		mDisplayRender = false;
	}
	else if (event->type == events.StartRaytrace.type)
	{
		std::println("start raytrace");

		SDL_CHECK(SDL_PushEvent(&events.RaytraceStarted));

		int* tmp_render_target_extent = mImGUIState->GetRenderTargetExtent();

		if (mRenderTargetExtent.width != tmp_render_target_extent[0] ||
			mRenderTargetExtent.height != tmp_render_target_extent[1])
		{
			mRenderTargetExtent.width = tmp_render_target_extent[0];
			mRenderTargetExtent.height = tmp_render_target_extent[1];
			RecreateRenderTarget();
		}

		int& tmp_max_samples = mImGUIState->GetMaxSamples();
		if (mMaxSamples != tmp_max_samples)
		{
			mMaxSamples = tmp_max_samples;
		}

		StartRaytracing();
	}
	else if (event->type == events.StopRaytrace.type)
	{
		std::println("stop raytrace");
		StopRaytracing();
	}
	else if (event->type == events.RaytraceStarted.type)
	{
		mDisplayRender = true;
	}

	mImGUIState->ProcessEvent(event);
}

void App::Iterate()
{
	if (mDisplayRender)
		RunDisplay();
	else
		RunRasterizer();
}

void App::RunRasterizer()
{
	mRasterizer->Render(mScene.get(), mImGUIState.get());
}

void App::RunDisplay()
{
	mDisplay->Render(mDeltaMousePosition, mZoomLevel, mImGUIState.get());
}

void App::StartRaytracing()
{
	mRaytraceThread = std::thread(&Raytracer::Start, mRaytracer.get(), mMaxSamples);
	mRaytraceThread.detach();
}

void App::RecreateRenderTarget()
{
	VK_CHECK("device wait idle", vkDeviceWaitIdle(mVulkanInterface->GetDevice()->GetDevice()));

	VkExtent3D extent = { mRenderTargetExtent.width, mRenderTargetExtent.height, 1 };
	std::vector<uint32_t> queue_family_indices{ mVulkanInterface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex,  mVulkanInterface->GetPhysicalDeviceData()->TransferQueueFamilyIndex };
	mFinalRenderTarget = std::make_unique<ImageResource>(mVulkanInterface->GetDevice()->GetDevice(),
		extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		mVulkanInterface->GetAllocator()->GetAllocator(), queue_family_indices,
		"final render target");

	Utils_InitializeImages({ mFinalRenderTarget->GetImage() }, mVulkanInterface->GetTransferObjects()->GetCommandBuffer(), mVulkanInterface->GetTransferObjects()->GetQueue());

	mDisplay->UpdateFinalRenderTarget(mFinalRenderTarget.get());
	mRaytracer->RecreateRenderResources(mRenderTargetExtent);
	mRaytracer->UpdateFinalRenderTarget(mFinalRenderTarget.get());
}

void App::StopRaytracing()
{
	mRaytracer->Stop();
}

void App::RecreateRasterSwapchain()
{
	mVulkanInterface->RecreateRasterSwapchain();
	mDisplay->UpdateSwapchain(mVulkanInterface->GetSwapchain());
	mDisplay->UpdateExtent(mVulkanInterface->GetSurfaceKHR()->GetSurfaceCapabilities().surfaceCapabilities.currentExtent);
	mRasterizer->UpdateSwapchain(mVulkanInterface->GetSwapchain());
	mRasterizer->UpdateExtent(mVulkanInterface->GetSurfaceKHR()->GetSurfaceCapabilities().surfaceCapabilities.currentExtent);
	mRasterizer->RecreateDepthTexture();
}

VulkanInterface* App::GetVulkanInterface() const
{
	return mVulkanInterface.get();
}

Rasterizer* App::GetRasterizer() const
{
	return mRasterizer.get();
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
	return mDisplayRender;
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
	StopRaytracing();

	VK_CHECK("device wait idle", vkDeviceWaitIdle(mVulkanInterface->GetDevice()->GetDevice()));
}
