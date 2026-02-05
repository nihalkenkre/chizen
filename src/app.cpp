#include "app.hpp"
#include "vulkan_interface.hpp"
#include "viewport.hpp"
#include "display.hpp"
#include "vulkan_raytracer.hpp"
#include "embree_raytracer.hpp"
#include "sw_rasterizer.hpp"
#include "imgui_state.hpp"
#include "utils.hpp"
#include "vulkan_objects.hpp"
#include "resources.hpp"
#include "scene.hpp"
#include "viewport_scene.hpp"
#include "embree_raytracer_scene.hpp"
#include "vulkan_raytracer_scene.hpp"
#include "sw_rasterizer_scene.hpp"
#include "vulkan_scene.hpp"

#include "events.hpp"

App::App(SDL_Window* window) : mWindow(window)
{
	mVulkanInterface = std::make_unique<VulkanInterface>(window);
	mImGUIState = std::make_unique<ImGUIState>(mVulkanInterface.get());
	mViewportScene = std::make_unique<ViewportEmptyScene>();

	VkExtent3D extent = VkExtent3D{ mFinalRenderTargetExtent.width, mFinalRenderTargetExtent.height, 1 };
	mFinalRenderTarget = std::make_unique<ImageResource>(mVulkanInterface->GetVkDevice(),
		extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		mVulkanInterface->GetVmaAllocator(),
		std::vector<uint32_t>{ mVulkanInterface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->TransferQueueFamilyIndex },
		"final render target"
	);

	mStagingRenderTarget = std::make_unique<HostBufferResource>(
		mVulkanInterface->GetVkDevice(), mVulkanInterface->GetVmaAllocator(),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
		mFinalRenderTargetExtent.width * mFinalRenderTargetExtent.height * 4 * sizeof(float), "staging render target",
		std::vector<uint32_t>{ mVulkanInterface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->TransferQueueFamilyIndex }
	);

	auto transfer_helpers = mVulkanInterface->GetTransferHelpers();
	transfer_helpers->RecordBatch();
	transfer_helpers->ChangeImageLayout(
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, VK_IMAGE_ASPECT_COLOR_BIT,
		mFinalRenderTarget->GetVkImage()
	);
	transfer_helpers->SubmitBatch();

	mViewport = std::make_unique<Viewport>(mVulkanInterface.get());
	mDisplay = std::make_unique<Display>(mVulkanInterface.get(), mFinalRenderTarget.get());
	mVulkanRaytracer = std::make_unique<VulkanRaytracer>(mVulkanInterface.get(), extent);
	mEmbreeRaytracer = std::make_unique<EmbreeRaytracer>();
	mSWRasterizer = std::make_unique<SWRasterizer>();
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
		RecreateViewportSwapchain();
	}
	else if (event->type == SDL_EVENT_KEY_DOWN)
	{
		if (event->key.key == SDLK_ESCAPE)
		{
			StopRendering();
		}
	}
	else if (event->type == events.FileOpen.type)
	{
		auto scene = Scene(
			reinterpret_cast<const char*>(event->user.data1),
			mVulkanInterface->GetPhysicalDeviceData()->Properties.properties.limits.minUniformBufferOffsetAlignment,
			mVulkanInterface->GetPhysicalDeviceData()->Properties.properties.limits.minStorageBufferOffsetAlignment
		);

		if (scene.GetMeshes().size() == 0)
		{
			std::println("No mesh data imported from file.");
		}
		else
		{
			mImGUIState->SetCameraNames(scene.GetCameraNames());

			mVulkanScene = std::make_unique<VulkanScene>(scene, mVulkanInterface.get(), mImGUIState->GetCPUShading());
			mViewportScene = std::make_unique<ViewportWorldScene>(mVulkanScene.get(), mVulkanInterface.get());
			mVulkanRaytracerScene = std::make_unique<VulkanRaytracerScene>(scene, mVulkanScene.get(), mVulkanInterface.get());
			mEmbreeRaytacerScene = std::make_unique<EmbreeRaytracerScene>(scene);
			mSWRasterizerScene = std::make_unique<SWRasterizerScene>(scene);

			mDisplayRender = false;
		}
	}
	else if (event->type == events.StartRender.type)
	{
		mRenderType = events.StartRender.user.code;

		if (mRenderType == 0)
		{
			const VkClearColorValue clear_color = {
				.float32 = {
					0, 0, 0, 1
				},
			};

			if (mVulkanRaytracerScene != nullptr)
			{
				int* tmp_render_target_extent = mImGUIState->GetFinalRenderTargetExtent();

				if (mFinalRenderTargetExtent.width != tmp_render_target_extent[0] ||
					mFinalRenderTargetExtent.height != tmp_render_target_extent[1])
				{
					mFinalRenderTargetExtent.width = tmp_render_target_extent[0];
					mFinalRenderTargetExtent.height = tmp_render_target_extent[1];
					RecreateRenderTarget();
				}

				int& tmp_max_samples = mImGUIState->GetMaxSamples();
				if (mMaxSamples != tmp_max_samples)
				{
					mMaxSamples = tmp_max_samples;
				}

				ComputeHelpers* compute_helpers = mVulkanInterface->GetComputeHelpers();
				compute_helpers->RecordBatch();
				compute_helpers->ClearImage(mVulkanRaytracer->GetAccumRenderTarget()->GetVkImage(), clear_color);
				compute_helpers->SubmitBatch();

				mRenderThread = std::thread(&VulkanRaytracer::Start, mVulkanRaytracer.get(), mVulkanRaytracerScene.get(), mFinalRenderTarget.get(), mFinalRenderTargetExtent, mMaxSamples, mImGUIState->GetSelectedCameraIndex(), mImGUIState->GetCPUShading());
				mRenderThread.detach();
				SDL_CHECK(SDL_PushEvent(&events.RenderStarted));
			}
		}
		else if (mRenderType == 1)
		{
			if (mEmbreeRaytacerScene != nullptr)
			{
				int* tmp_render_target_extent = mImGUIState->GetFinalRenderTargetExtent();

				if (mFinalRenderTargetExtent.width != tmp_render_target_extent[0] ||
					mFinalRenderTargetExtent.height != tmp_render_target_extent[1])
				{
					mFinalRenderTargetExtent.width = tmp_render_target_extent[0];
					mFinalRenderTargetExtent.height = tmp_render_target_extent[1];
					RecreateRenderTarget();
				}

				int& tmp_max_samples = mImGUIState->GetMaxSamples();
				if (mMaxSamples != tmp_max_samples)
				{
					mMaxSamples = tmp_max_samples;
				}

				std::memset(
					mStagingRenderTarget->GetAllocationInfo2().allocationInfo.pMappedData,
					0,
					mStagingRenderTarget->GetAllocationInfo2().allocationInfo.size
				);

				TransferHelpers* transfer_helpers = mVulkanInterface->GetTransferHelpers();
				transfer_helpers->RecordBatch();
				transfer_helpers->CopyBufferToImage(mStagingRenderTarget->GetVkBuffer(), mFinalRenderTarget->GetVkImage(), mFinalRenderTargetExtent);
				transfer_helpers->SubmitBatch();

				mRenderThread = std::thread(&EmbreeRaytracer::Start, mEmbreeRaytracer.get(), mEmbreeRaytacerScene.get(), mFinalRenderTargetExtent.width, mFinalRenderTargetExtent.height, mMaxSamples, mImGUIState->GetSelectedCameraIndex(), reinterpret_cast<float*>(mStagingRenderTarget->GetAllocationInfo2().allocationInfo.pMappedData));
				mRenderThread.detach();

				SDL_CHECK(SDL_PushEvent(&events.RenderStarted));
			}
		}
		else if (mRenderType == 2)
		{
			if (mSWRasterizerScene != nullptr)
			{
				int* tmp_render_target_extent = mImGUIState->GetFinalRenderTargetExtent();

				if (mFinalRenderTargetExtent.width != tmp_render_target_extent[0] ||
					mFinalRenderTargetExtent.height != tmp_render_target_extent[1])
				{
					mFinalRenderTargetExtent.width = tmp_render_target_extent[0];
					mFinalRenderTargetExtent.height = tmp_render_target_extent[1];
					RecreateRenderTarget();
				}

				int& tmp_max_samples = mImGUIState->GetMaxSamples();
				if (mMaxSamples != tmp_max_samples)
				{
					mMaxSamples = tmp_max_samples;
				}

				std::memset(
					mStagingRenderTarget->GetAllocationInfo2().allocationInfo.pMappedData,
					0,
					mStagingRenderTarget->GetAllocationInfo2().allocationInfo.size
				);

				TransferHelpers* transfer_helpers = mVulkanInterface->GetTransferHelpers();
				transfer_helpers->RecordBatch();
				transfer_helpers->CopyBufferToImage(mStagingRenderTarget->GetVkBuffer(), mFinalRenderTarget->GetVkImage(), mFinalRenderTargetExtent);
				transfer_helpers->SubmitBatch();

				mRenderThread = std::thread(
					&SWRasterizer::Start,
					mSWRasterizer.get(),
					mSWRasterizerScene.get(),
					mFinalRenderTargetExtent.width,
					mFinalRenderTargetExtent.height,
					mMaxSamples,
					mImGUIState->GetSelectedCameraIndex(),
					reinterpret_cast<float*>(mStagingRenderTarget->GetAllocationInfo2().allocationInfo.pMappedData)
				);
				mRenderThread.detach();

				SDL_CHECK(SDL_PushEvent(&events.RenderStarted));
			}
		}
	}
	else if (event->type == events.StopRender.type)
	{
		StopRendering();
	}
	else if (event->type == events.RenderStarted.type)
	{
		mDisplayRender = true;
	}
	else if (event->type == events.RenderStopped.type)
	{
		TransferHelpers* transfer_helpers = mVulkanInterface->GetTransferHelpers();
		if (mRenderType == 1 || mRenderType == 2)
		{
			transfer_helpers->RecordBatch();
			transfer_helpers->InsertMemoryBarrier(
				VK_PIPELINE_STAGE_2_HOST_BIT, VK_ACCESS_2_MEMORY_WRITE_BIT,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT
			);
			transfer_helpers->CopyBufferToImage(
				mStagingRenderTarget->GetVkBuffer(), mFinalRenderTarget->GetVkImage(), mFinalRenderTargetExtent
			);
			transfer_helpers->SubmitBatch();
		}
		mDisplayRender = true;
	}
	else if (event->type == events.RenderSampleDone.type)
	{
		if (mRenderType == 1 || mRenderType == 2)
		{
			TransferHelpers* transfer_helpers = mVulkanInterface->GetTransferHelpers();
			transfer_helpers->RecordBatch();
			transfer_helpers->CopyBufferToImage(
				mStagingRenderTarget->GetVkBuffer(), mFinalRenderTarget->GetVkImage(), mFinalRenderTargetExtent
			);
			transfer_helpers->SubmitBatch();
		}
		mDisplayRender = true;
	}
	else if (event->type == events.ReloadShaders.type)
	{
		VK_CHECK("device wait idle", vkDeviceWaitIdle(mVulkanInterface->GetVkDevice()));
		if (mViewportScene != nullptr)
			mViewportScene->ReloadShaders();

		if (mDisplay != nullptr)
			mDisplay->ReloadShaders();

		if (mVulkanRaytracerScene != nullptr)
			mVulkanRaytracerScene->ReloadShaders();
	}

	mImGUIState->ProcessEvent(event);
}

void App::Iterate()
{
	if (mDisplayRender)
		RunDisplay();
	else
		RunViewport();
}

void App::RunViewport()
{
	mViewport->Render(mViewportScene.get(), mVulkanInterface->GetSwapchain(),
		mVulkanInterface->GetSurfaceKHR()->GetSurfaceCapabilities().surfaceCapabilities.currentExtent,
		mImGUIState.get(),
		mVulkanInterface->GetTransferHelpers()
	);
}

void App::RunDisplay()
{
	mDisplay->Render(mVulkanInterface->GetSwapchain(),
		mVulkanInterface->GetSurfaceKHR()->GetSurfaceCapabilities().surfaceCapabilities.currentExtent,
		mDeltaMousePosition, mZoomLevel, mImGUIState.get(), mVulkanInterface->GetComputeHelpers()
	);
}

void App::RecreateRenderTarget()
{
	VK_CHECK("device wait idle", vkDeviceWaitIdle(mVulkanInterface->GetVkDevice()));

	VkExtent3D extent = { mFinalRenderTargetExtent.width, mFinalRenderTargetExtent.height, 1 };
	std::vector<uint32_t> queue_family_indices{ mVulkanInterface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex,  mVulkanInterface->GetPhysicalDeviceData()->TransferQueueFamilyIndex };
	mFinalRenderTarget = std::make_unique<ImageResource>(mVulkanInterface->GetVkDevice(),
		extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		mVulkanInterface->GetVmaAllocator(),
		queue_family_indices,
		"final render target"
	);

	mStagingRenderTarget = std::make_unique<HostBufferResource>(
		mVulkanInterface->GetVkDevice(), mVulkanInterface->GetVmaAllocator(),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
		mFinalRenderTargetExtent.width * mFinalRenderTargetExtent.height * 4 * sizeof(float),
		"staging render target",
		std::vector<uint32_t>{ mVulkanInterface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex, mVulkanInterface->GetPhysicalDeviceData()->TransferQueueFamilyIndex }
	);

	auto transfer_helpers = mVulkanInterface->GetTransferHelpers();
	transfer_helpers->RecordBatch();
	transfer_helpers->ChangeImageLayout(
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, VK_IMAGE_ASPECT_COLOR_BIT,
		mFinalRenderTarget->GetVkImage());
	transfer_helpers->SubmitBatch();

	mDisplay->UpdateFinalRenderTargetDesc(mFinalRenderTarget.get());
	mVulkanRaytracer->RecreateRenderResources(mFinalRenderTargetExtent);
}

void App::StopRendering()
{
	if (mRenderType == 0)
	{
		mVulkanRaytracer->Stop();
	}
	else if (mRenderType == 1)
	{
		mEmbreeRaytracer->Stop();
	}
	else if (mRenderType == 2)
	{
		mSWRasterizer->Stop();
	}
}

void App::RecreateViewportSwapchain()
{
	mVulkanInterface->RecreateViewportSwapchain();
	mViewport->RecreateDepthTexture(mVulkanInterface->GetSurfaceKHR()->GetSurfaceCapabilities().surfaceCapabilities.currentExtent, mVulkanInterface->GetTransferHelpers());
}

VulkanInterface* App::GetVulkanInterface() const
{
	return mVulkanInterface.get();
}

Viewport* App::GetViewport() const
{
	return mViewport.get();
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

ImGUIState* App::GetImGUIState() const
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

VkExtent3D& App::GetFinalRenderTargetExtent()
{
	return mFinalRenderTargetExtent;
}

ImageResource* App::GetFinalRenderTarget() const
{
	return mFinalRenderTarget.get();
}

App::~App() noexcept
{
	StopRendering();
}
