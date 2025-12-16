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
#include "rasterizer_scene.hpp"
#include "embree_raytracer_scene.hpp"
#include "vulkan_raytracer_scene.hpp"
#include "events.hpp"

App::App(SDL_Window* window, const std::string& current_path) : mWindow(window)
{
	mVulkanInterface = std::make_unique<VulkanInterface>(window);
	mImGUIState = std::make_unique<ImGUIState>(mVulkanInterface.get());
	mRasterizerScene = std::make_unique<RasterizeEmptyScene>();

	VkExtent3D extent = VkExtent3D{ mRenderTargetExtent.width, mRenderTargetExtent.height, 1 };
	mFinalRenderTarget = std::make_unique<ImageResource>(mVulkanInterface->GetVkDevice(),
		extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		mVulkanInterface->GetVmaAllocator(),
		std::vector<uint32_t>{ mVulkanInterface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->TransferQueueFamilyIndex },
		"final render target");

	mEmbreeRenderTarget = std::make_unique<HostBufferResource>(
		mVulkanInterface->GetVkDevice(), mVulkanInterface->GetVmaAllocator(),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
		mRenderTargetExtent.width * mRenderTargetExtent.height * 4 * sizeof(float), "embree render target"
	);

	auto transfer_objects = mVulkanInterface->GetTransferObjects();
	transfer_objects->BeginBatch();
	transfer_objects->ChangeImageLayout(
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, VK_IMAGE_ASPECT_COLOR_BIT,
		mFinalRenderTarget->GetImage());
	transfer_objects->EndBatch();

	mRasterizer = std::make_unique<Rasterizer>(mVulkanInterface.get(), current_path, "rasterizer");
	mDisplay = std::make_unique<Display>(mVulkanInterface.get(), mFinalRenderTarget.get(), current_path);
	mVulkanRaytracer = std::make_unique<VulkanRaytracer>(mVulkanInterface.get(), mFinalRenderTarget.get(), extent, current_path, "raytrace");
	mEmbreeRaytracer = std::make_unique<EmbreeRaytracer>();
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
			mVulkanInterface->GetVkDevice(),
			mVulkanInterface->GetVmaAllocator(),
			mRasterizer->GetDescriptorSetLayouts(),
			mVulkanInterface->GetTransferObjects()
		);

		mVulkanRaytracerScene = std::make_unique<VulkanRaytracerScene>(scene, mVulkanInterface->GetComputeHelpers()->GetCommandBuffer(), mVulkanInterface->GetComputeHelpers()->GetQueue());
		//mEmbreeRaytacerScene = std::make_unique<EmbreeRaytracerScene>(scene);

		mDisplayRender = false;
	}
	else if (event->type == events.StartRaytrace.type)
	{
		mRaytracerType = events.StartRaytrace.user.code;

		if (mRaytracerType == 0)
		{
			if (mVulkanRaytracerScene != nullptr)
			{
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

				mRaytraceThread = std::thread(&VulkanRaytracer::Start, mVulkanRaytracer.get(), mVulkanRaytracerScene.get(), mMaxSamples);
				mRaytraceThread.detach();
				SDL_CHECK(SDL_PushEvent(&events.RaytraceStarted));
			}
		}
		else if (mRaytracerType == 1)
		{
			if (mEmbreeRaytacerScene != nullptr)
			{
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

				mRaytraceThread = std::thread(&EmbreeRaytracer::Start, mEmbreeRaytracer.get(), mEmbreeRaytacerScene.get(), mMaxSamples, reinterpret_cast<float*>(mEmbreeRenderTarget->GetAllocationInfo2().allocationInfo.pMappedData));
				mRaytraceThread.detach();
				SDL_CHECK(SDL_PushEvent(&events.RaytraceStarted));
			}
		}
	}
	else if (event->type == events.StopRaytrace.type)
	{
		StopRaytracing();
	}
	else if (event->type == events.RaytraceStarted.type)
	{
		mDisplayRender = true;
	}
	else if (event->type == events.RaytraceStopped.type)
	{
		if (mRaytracerType == 1)
		{
			mVulkanInterface->GetTransferObjects()->BeginBatch();
			mVulkanInterface->GetTransferObjects()->CopyBufferToImage(
				mEmbreeRenderTarget->GetVkBuffer(), mFinalRenderTarget->GetImage(), mRenderTargetExtent
			);
			mVulkanInterface->GetTransferObjects()->EndBatch();
		}
	}
	else if (event->type == events.RaytraceSampleDone.type)
	{
		if (mRaytracerType == 1)
		{
			mVulkanInterface->GetTransferObjects()->BeginBatch();
			mVulkanInterface->GetTransferObjects()->CopyBufferToImage(
				mEmbreeRenderTarget->GetVkBuffer(), mFinalRenderTarget->GetImage(), mRenderTargetExtent
			);
			mVulkanInterface->GetTransferObjects()->EndBatch();
		}
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

void App::RecreateRenderTarget()
{
	VK_CHECK("device wait idle", vkDeviceWaitIdle(mVulkanInterface->GetVkDevice()));

	VkExtent3D extent = { mRenderTargetExtent.width, mRenderTargetExtent.height, 1 };
	std::vector<uint32_t> queue_family_indices{ mVulkanInterface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex,  mVulkanInterface->GetPhysicalDeviceData()->TransferQueueFamilyIndex };
	mFinalRenderTarget = std::make_unique<ImageResource>(mVulkanInterface->GetVkDevice(),
		extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		mVulkanInterface->GetVmaAllocator(),
		queue_family_indices,
		"final render target");

	mEmbreeRenderTarget = std::make_unique<HostBufferResource>(
		mVulkanInterface->GetVkDevice(), mVulkanInterface->GetVmaAllocator(),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
		mRenderTargetExtent.width * mRenderTargetExtent.height * 4 * sizeof(float),
		"embree render target"
	);

	auto transfer_objects = mVulkanInterface->GetTransferObjects();
	transfer_objects->BeginBatch();
	transfer_objects->ChangeImageLayout(
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, VK_IMAGE_ASPECT_COLOR_BIT,
		mFinalRenderTarget->GetImage());
	transfer_objects->EndBatch();

	mDisplay->UpdateFinalRenderTarget(mFinalRenderTarget.get());

	mEmbreeRaytracer->RecreateRenderResources(extent.width, extent.height);

	mVulkanRaytracer->RecreateRenderResources(mRenderTargetExtent);
	mVulkanRaytracer->UpdateFinalRenderTarget(mFinalRenderTarget.get());
}

void App::StopRaytracing()
{
	if (mRaytracerType == 0)
	{
		mVulkanRaytracer->Stop();

	}
	else if (mRaytracerType == 1)
	{
		mEmbreeRaytracer->Stop();
	}
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

	VK_CHECK("device wait idle", vkDeviceWaitIdle(mVulkanInterface->GetVkDevice()));
}
