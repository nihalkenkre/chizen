#include "vulkan_raytracer.hpp"
#include "frame_objects.hpp"
#include "utils.hpp"
#include "vulkan_interface.hpp"
#include "resources.hpp"
#include "vulkan_objects.hpp"
#include "events.hpp"
#include "vulkan_raytracer_scene.hpp"

VulkanRaytracer::VulkanRaytracer(const VulkanInterface* const vulkan_interface, const VkExtent3D& extent, const std::string& current_path, const std::string& name)
	: mDevice(vulkan_interface->GetVkDevice()), mAllocator(vulkan_interface->GetVmaAllocator())
{
	mDevice = vulkan_interface->GetVkDevice();
	mRayTracingProperties = vulkan_interface->GetPhysicalDeviceData()->RayTracingProperties;
	mQueueFamilyIndices = {
		vulkan_interface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex,
		vulkan_interface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex,
		vulkan_interface->GetPhysicalDeviceData()->TransferQueueFamilyIndex
	};
	mAccumRenderTarget = std::make_unique<ImageResource>(
		mDevice, extent, VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, vulkan_interface->GetVmaAllocator(), mQueueFamilyIndices, "accum render target");
	mMaxFramesInFlight = static_cast<uint8_t>(vulkan_interface->GetSwapchain()->GetImagesCount());
	mComputeQueue = vulkan_interface->GetDevice()->GetComputeQueue();
	mTransferHelpers = vulkan_interface->GetTransferHelpers();
	mFrameObjects = std::make_unique<FrameObjects>(mDevice, vulkan_interface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex, mMaxFramesInFlight, "raytrace frame objects");

	InitializeResources(extent);
}

VulkanRaytracer::~VulkanRaytracer()
{
}

void VulkanRaytracer::InitializeResources(const VkExtent3D& extent)
{
	mTransferHelpers->RecordBatch();
	mTransferHelpers->ChangeImageLayout(
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		VK_IMAGE_ASPECT_COLOR_BIT,
		mAccumRenderTarget->GetVkImage()
	);

	std::vector<uint32_t> rand_states(4 * extent.width * extent.height);

	for (uint32_t st = 0; st < 4 * extent.width * extent.height; ++st)
	{
		uint32_t rand_val = rand();
		while (rand_val < 128)
			rand_val = rand();

		rand_states[st] = rand_val;
	}

	std::vector<uint8_t>rand_states_data(rand_states.size() * sizeof(uint32_t));
	std::memcpy(rand_states_data.data(), rand_states.data(), rand_states_data.size());

	mRandomStates = std::make_unique<DeviceBufferResource>(
		mDevice, mAllocator,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		rand_states_data.size(), "rand states"
	);

	auto rand_states_staging = std::make_unique<HostBufferResource>(
		mDevice, mAllocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		rand_states_data, "random states staging"
	);

	mTransferHelpers->CopyBufferToBuffer(rand_states_staging->GetVkBuffer(), mRandomStates->GetVkBuffer(), rand_states_data.size());
	mTransferHelpers->SubmitBatch();
}

void VulkanRaytracer::RecreateRenderResources(const VkExtent3D& extent)
{
	mAccumRenderTarget = std::make_unique<ImageResource>(mDevice, extent, VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, mAllocator,
		mQueueFamilyIndices, "accum render target");

	InitializeResources(extent);
}

void VulkanRaytracer::Start(const VulkanRaytracerScene* scene, const ImageResource* final_render_target, const VkExtent3D& extent, const uint32_t max_samples, const uint32_t cam_index)
{
	VkDevice device = mDevice;
	VkCommandBuffer cmd_buff = mFrameObjects->GetCommandBuffer();
	VkSemaphore frame_sem = mFrameObjects->GetSemaphore();
	uint64_t& frame_sem_value = mFrameObjects->GetFrameSemValue();
	uint8_t frame_in_flight = mFrameObjects->GetFrameInFlight();
	uint32_t s = 1;

	do {
		const VkSemaphoreWaitInfo wait_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &frame_sem,
			.pValues = &frame_sem_value,
		};

		VK_CHECK("wait acq img", vkWaitSemaphoresKHR(device, &wait_info, UINT64_MAX));

		// waiting for last submitted buffer to complete before exiting. resources in use.
		if (mStopRendering) break;

		const VkCommandBufferBeginInfo rt_begin_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};
		VK_CHECK("begin rt cmd_buff", vkBeginCommandBuffer(cmd_buff, &rt_begin_info));

		Utils_InsertMemoryBarrier(
			cmd_buff,
			VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR | VK_PIPELINE_STAGE_2_CLEAR_BIT,
			VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT
		);

		// look to clear image in app.cpp, RenderStarted event.
		if (s == 1)
		{
			const VkClearColorValue clear_color = {
				.float32 = {
					0, 0, 0, 1,
				},
			};

			const VkImageSubresourceRange ranges[] = {
				{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.levelCount = 1,
					.layerCount = 1,
				},
			};

			vkCmdClearColorImage(cmd_buff, mAccumRenderTarget->GetVkImage(), VK_IMAGE_LAYOUT_GENERAL, &clear_color, std::size(ranges), ranges);
		}

		scene->Render(cmd_buff, mRandomStates.get(), mAccumRenderTarget.get(), final_render_target, s, extent.width, extent.height, cam_index);

		VK_CHECK("end rt cmd buffer", vkEndCommandBuffer(cmd_buff));

		const VkCommandBufferSubmitInfo rt_cmd_buff_infos[] = {
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
				.commandBuffer = cmd_buff,
			},
		};

		const VkSemaphoreSubmitInfo rt_sig_sem_infos[] = {
			{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.semaphore = frame_sem,
				.value = ++frame_sem_value,
				.stageMask = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
			},
		};

		const VkSubmitInfo2 rt_submit_infos[] = {
			{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
				.commandBufferInfoCount = std::size(rt_cmd_buff_infos),
				.pCommandBufferInfos = rt_cmd_buff_infos,
				.signalSemaphoreInfoCount = std::size(rt_sig_sem_infos),
				.pSignalSemaphoreInfos = rt_sig_sem_infos,
			},
		};

		VK_CHECK("submit rt commamds", vkQueueSubmit2KHR(mComputeQueue, std::size(rt_submit_infos), rt_submit_infos, VK_NULL_HANDLE));

		mFrameObjects->NextFrame();
	} while (++s <= max_samples);

	// waiting for last submitted buffer to complete before exiting. resources in use.
	VK_CHECK("raytrace queue wait idle", vkQueueWaitIdle(mComputeQueue));

	mStopRendering = false;

	SDL_CHECK(SDL_PushEvent(&events.RenderStopped));
}

void VulkanRaytracer::Stop()
{
	mStopRendering = true;
}

FrameObjects* VulkanRaytracer::GetFrameObjects() const
{
	return mFrameObjects.get();
}

