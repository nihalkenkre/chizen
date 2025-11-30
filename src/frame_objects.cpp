#include "frame_objects.hpp"
#include "utils.hpp"
#include "vulkan_functions.hpp"

FrameObjects::FrameObjects(const VkDevice device, const uint32_t queue_family_index, const uint8_t& max_frames_in_flight)
{
	mMaxFramesInFlight = max_frames_in_flight;
	mCommandBuffers.resize(max_frames_in_flight, VK_NULL_HANDLE);
	mFrameSemaphores.resize(max_frames_in_flight, VK_NULL_HANDLE);
	mFrameSemaphoreValues.resize(max_frames_in_flight, 1);
	mDevice = device;

	const VkSemaphoreTypeCreateInfo sem_type_ci = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
		.initialValue = 0,
	};

	const VkSemaphoreCreateInfo tl_sem_ci = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = &sem_type_ci,
	};

	VkSemaphoreSignalInfo sem_sig_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
		.value = 1,
	};

	const VkCommandPoolCreateInfo cmd_pool_ci = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = queue_family_index,
	};

	VK_CHECK("create command pool", vkCreateCommandPool(mDevice, &cmd_pool_ci, nullptr, &mCommandPool));

	const VkCommandBufferAllocateInfo cmd_buff_ai = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = mCommandPool,
		.commandBufferCount = mMaxFramesInFlight,
	};

	VK_CHECK("allocator cmd buffs", vkAllocateCommandBuffers(mDevice, &cmd_buff_ai, mCommandBuffers.data()));

	for (uint8_t fr = 0; fr < mMaxFramesInFlight; ++fr)
	{
		VK_CHECK("create frame semaphore", vkCreateSemaphore(mDevice, &tl_sem_ci, nullptr, mFrameSemaphores.data() + fr));
		sem_sig_info.semaphore = mFrameSemaphores[fr];

		VK_CHECK("signal frame semaphore", vkSignalSemaphoreKHR(mDevice, &sem_sig_info));
	}
}

uint8_t FrameObjects::GetFrameInFlight() const
{
	return mFrameInFlight;
}

VkCommandBuffer FrameObjects::GetCommandBuffer() const
{
	return mCommandBuffers[mFrameInFlight];
}

VkSemaphore FrameObjects::GetSemaphore() const
{
	return mFrameSemaphores[mFrameInFlight];
}

uint64_t& FrameObjects::GetFrameSemValue()
{
	return mFrameSemaphoreValues[mFrameInFlight];
}

void FrameObjects::NextFrame()
{
	mFrameInFlight = (mFrameInFlight + 1) % mMaxFramesInFlight;
}

FrameObjects::~FrameObjects() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
	{
		for (uint8_t fr = 0; fr < mMaxFramesInFlight; ++fr)
		{
			vkDestroySemaphore(mDevice, mFrameSemaphores[fr], nullptr);
		}
		vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
	}
}
