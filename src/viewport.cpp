#include "viewport.hpp"
#include "vulkan_interface.hpp"
#include "vulkan_objects.hpp"
#include "frame_objects.hpp"
#include "imgui_state.hpp"
#include "resources.hpp"
#include "scene.hpp"
#include "viewport_scene.hpp"

#include "utils.hpp"

Viewport::Viewport(const VulkanInterface* vulkan_interface, const std::string& current_path, const std::string& name) :
	mDevice(vulkan_interface->GetVkDevice()),
	mQueue(vulkan_interface->GetDevice()->GetGraphicsQueue()),
	mAllocator(vulkan_interface->GetAllocator()),
	mTransferHelpers(vulkan_interface->GetTransferHelpers()),
	mQueueFamilyIndices({
			vulkan_interface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex,
			vulkan_interface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex,
			vulkan_interface->GetPhysicalDeviceData()->TransferQueueFamilyIndex,
		})
{
	mMaxFramesInFlight = static_cast<uint8_t>(vulkan_interface->GetSwapchain()->GetImagesCount()) + 2;
	mFrameObjects = std::make_unique<FrameObjects>(vulkan_interface->GetVkDevice(), vulkan_interface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex, mMaxFramesInFlight, "rasterizer frame objects");
	mDepthTexture = std::make_unique<ImageResource>(
		vulkan_interface->GetVkDevice(),
		VkExtent3D{
				vulkan_interface->GetSurfaceKHR()->GetSurfaceCapabilities().surfaceCapabilities.currentExtent.width,
				vulkan_interface->GetSurfaceKHR()->GetSurfaceCapabilities().surfaceCapabilities.currentExtent.height,
			1,
		},
		VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, vulkan_interface->GetVmaAllocator(),
		mQueueFamilyIndices,
		"raster depth texture"
		);

	mAcquireSignalSemaphores.resize(mMaxFramesInFlight, VK_NULL_HANDLE);
	mPresentWaitSemaphores.resize(mMaxFramesInFlight, VK_NULL_HANDLE);

	const VkSemaphoreCreateInfo bin_sem_ci = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	for (uint8_t fr = 0; fr < mMaxFramesInFlight; ++fr)
	{
		VK_CHECK("create acq sig semaphore", vkCreateSemaphore(mDevice, &bin_sem_ci, nullptr, mAcquireSignalSemaphores.data() + fr));
		VK_CHECK("create present wait semaphore", vkCreateSemaphore(mDevice, &bin_sem_ci, nullptr, mPresentWaitSemaphores.data() + fr));

#ifdef _DEBUG
		Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(mAcquireSignalSemaphores[fr]), std::string("raster acq sig sem ").append(std::to_string(fr).c_str()));
		Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(mPresentWaitSemaphores[fr]), std::string("raster prsnt wait sem ").append(std::to_string(fr).c_str()));
#endif // _DEBUG
	}

	mTransferHelpers->RecordBatch();
	mTransferHelpers->ChangeImageLayout(
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		mDepthTexture->GetImage()
	);
	mTransferHelpers->SubmitBatch();
}

void Viewport::Render(const ViewportScene* scene, const Swapchain* swapchain, const VkExtent2D extent, ImGUIState* imgui_state)
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
		.swapchain = swapchain->GetSwapchain(),
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
		VK_IMAGE_ASPECT_COLOR_BIT,
		swapchain->GetImages()[img_idx]
	);

	VkRenderingAttachmentInfo col_attachs[] = {
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = swapchain->GetImageViews()[img_idx],
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

	VkRenderingAttachmentInfo depth_attachment_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = mDepthTexture->GetDescriptorInfo().imageView,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = {
			.depthStencil = {
				.depth = 1.f,
			},
		},
	};

	const VkRenderingInfo rendering_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = {
			.extent = extent,
		},
		.layerCount = 1,
		.colorAttachmentCount = std::size(col_attachs),
		.pColorAttachments = col_attachs,
		.pDepthAttachment = &depth_attachment_info,
	};

	vkCmdBeginRenderingKHR(cmd_buff, &rendering_info);

	const VkViewport viewports[] = {
		{
			.width = 1920.f,//static_cast<float>(mExtent.width),
			.height = 1080.f,//static_cast<float>(mExtent.height),
			.minDepth = 0.f,
			.maxDepth = 1.f,
		},
	};

	const VkRect2D scissors[] = {
		{
			.extent = extent,
		},
	};

	vkCmdSetScissor(cmd_buff, 0, std::size(scissors), scissors);
	vkCmdSetViewport(cmd_buff, 0, std::size(viewports), viewports);

	scene->Render(cmd_buff, imgui_state->GetSelectedCameraIndex());
	imgui_state->Render(cmd_buff);

	vkCmdEndRenderingKHR(cmd_buff);

	Utils_ChangeImageLayout(cmd_buff,
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		VK_IMAGE_ASPECT_COLOR_BIT,
		swapchain->GetImages()[img_idx]
	);

	VK_CHECK("end rasterizer cmd buff", vkEndCommandBuffer(cmd_buff));

	const VkSemaphoreSubmitInfo wait_sem_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = mAcquireSignalSemaphores[frame_in_flight],
			.stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		},
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = mTransferHelpers->GetSemaphore(),
			.value = mTransferHelpers->GetSemaphoreValue(),
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

	VK_CHECK("submit viewport render commands", vkQueueSubmit2KHR(mQueue, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));

	VkSwapchainKHR sc = swapchain->GetSwapchain();
	const VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = mPresentWaitSemaphores.data() + frame_in_flight,
		.swapchainCount = 1,
		.pSwapchains = &sc,
		.pImageIndices = &img_idx,
	};

	VK_CHECK("q present", vkQueuePresentKHR(mQueue, &present_info));

	// vkQueueWaitIdle is required for the compute queue to fly. 
	// Else cmpt queue with the gfx queue, WIERD!!!
	// Need to check
	VK_CHECK("gfx q wait idle", vkQueueWaitIdle(mQueue));

	mFrameObjects->NextFrame();
}

void Viewport::RecreateDepthTexture(const VkExtent2D extent)
{
	mDepthTexture = std::make_unique<ImageResource>(
		mDevice,
		VkExtent3D{
			extent.width,
			extent.height,
			1,
		},
		VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, mAllocator->GetAllocator(),
		mQueueFamilyIndices,
		"raster depth texture"
		);
}

Viewport::~Viewport() noexcept
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
