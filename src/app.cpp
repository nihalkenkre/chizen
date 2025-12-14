#include "app.hpp"
#include "vulkan_interface.hpp"
#include "rasterizer.hpp"
#include "display.hpp"
#include "vulkan_raytracer.hpp"
#include "embree_raytracer.hpp"
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
	mRasterizerScene = std::make_unique<RasterizeEmptyScene>();

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
	mVulkanRaytracer = std::make_unique<VulkanRaytracer>(mVulkanInterface.get(), mFinalRenderTarget.get(), extent, current_path, "raytrace");
}

void App::ProcessEvent(SDL_Event* event)
{
	ImGuiIO& io = ImGui::GetIO();

	if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
		!io.WantCaptureMouse)
	{
		if (event->button.button == 1)
		{
			mIsTrackingMouse = true;

			mLastMousePosition[0] = event->motion.x;
			mLastMousePosition[1] = event->motion.y;
		}
		else if (event->button.button == 2)
		{
			mDisplayRender = !mDisplayRender;
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_MOTION &&
		!io.WantCaptureMouse &&
		mIsTrackingMouse)
	{
		mDeltaMousePosition[0] += ((mLastMousePosition[0] - event->motion.x) / 1920.f) * 2;
		mDeltaMousePosition[1] += ((mLastMousePosition[1] - event->motion.y) / 1080.f) * 2;

		mLastMousePosition[0] = event->motion.x;
		mLastMousePosition[1] = event->motion.y;
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP &&
		event->button.button == 1 &&
		!io.WantCaptureMouse)
	{
		mLastMousePosition[0] = 0;
		mLastMousePosition[1] = 0;

		mIsTrackingMouse = false;
	}
	else if (event->type == SDL_EVENT_MOUSE_WHEEL)
	{
		mZoomLevel = std::max(0.01f, mZoomLevel + event->wheel.y / 20.f);
	}
	else if (event->type == SDL_EVENT_WINDOW_RESIZED)
	{
		RecreateRasterSwapchain();
	}
	else if (event->type == SDL_EVENT_KEY_DOWN)
	{
		if (event->key.key == SDLK_ESCAPE)
		{
			StopRaytracing();
		}
	}
	else if (event->type == events.FileOpen.type)
	{
		auto scene = Scene(
			reinterpret_cast<const char*>(event->user.data1),
			mVulkanInterface->GetPhysicalDeviceData()->Properties.properties.limits.minUniformBufferOffsetAlignment
		);

		mImGUIState->SetCameraNames(scene.GetCameraNames());

		mRasterizerScene = std::make_unique<RasterizerWorldScene>(
			scene,
			mVulkanInterface->GetDevice()->GetDevice(),
			mVulkanInterface->GetAllocator()->GetAllocator(),
			mRasterizer->GetDescriptorSetLayouts(),
			mVulkanInterface->GetTransferObjects()->GetCommandBuffer(),
			mVulkanInterface->GetTransferObjects()->GetQueue()
		);
		mVulkanRaytracerScene = std::make_unique<VulkanRaytracerScene>();
		mEmbreeRaytacerScene = std::make_unique<EmbreeRaytracerScene>();

		mDisplayRender = false;
	}
	else if (event->type == events.StartRaytrace.type)
	{
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

		StartRaytracing(events.StartRaytrace.user.code);
	}
	else if (event->type == events.StopRaytrace.type)
	{
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
	mRasterizer->Render(mRasterizerScene.get(), mImGUIState.get());
}

void App::RunDisplay()
{
	mDisplay->Render(mDeltaMousePosition, mZoomLevel, mImGUIState.get());
}

void App::StartRaytracing(const uint32_t raytracer_type)
{
	mRaytraceThread = std::thread(&VulkanRaytracer::Start, mVulkanRaytracer.get(), mMaxSamples);
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
	mVulkanRaytracer->RecreateRenderResources(mRenderTargetExtent);
	mVulkanRaytracer->UpdateFinalRenderTarget(mFinalRenderTarget.get());
}

void App::StopRaytracing()
{
	mVulkanRaytracer->Stop();
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
