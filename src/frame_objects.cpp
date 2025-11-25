#include "frame_objects.hpp"
#include "utils.hpp"

struct FrameObjects
{
	std::vector<VkSemaphore> FrameSemaphores;
	std::vector<uint64_t> FrameSemaphoreValues;
	std::vector<VkCommandBuffer> CommandBuffers;
	VkCommandPool CommandPool = VK_NULL_HANDLE;

	uint8_t MaxFramesInFlight = 0;
	uint8_t FrameInFlight = 0;

	VkDevice Device = VK_NULL_HANDLE;
};

FrameObjects* FrameObjects_Create(const VkDevice device, const uint32_t queue_family_index, const uint8_t& max_frames_in_flight)
{
	FrameObjects* fo = reinterpret_cast<FrameObjects*>(std::calloc(1, sizeof(FrameObjects)));

	fo->MaxFramesInFlight = max_frames_in_flight;
	fo->CommandBuffers.resize(max_frames_in_flight, VK_NULL_HANDLE);
	fo->FrameSemaphores.resize(max_frames_in_flight, VK_NULL_HANDLE);
	fo->FrameSemaphoreValues.resize(max_frames_in_flight, 1);
	fo->Device = device;

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

	VK_CHECK("create command pool", vkCreateCommandPool(fo->Device, &cmd_pool_ci, nullptr, &fo->CommandPool));

	const VkCommandBufferAllocateInfo cmd_buff_ai = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = fo->CommandPool,
		.commandBufferCount = fo->MaxFramesInFlight,
	};

	VK_CHECK("allocator cmd buffs", vkAllocateCommandBuffers(fo->Device, &cmd_buff_ai, fo->CommandBuffers.data()));

	for (uint8_t fr = 0; fr < fo->MaxFramesInFlight; ++fr)
	{
		VK_CHECK("create frame semaphore", vkCreateSemaphore(fo->Device, &tl_sem_ci, nullptr, fo->FrameSemaphores.data() + fr));
		sem_sig_info.semaphore = fo->FrameSemaphores[fr];

		VK_CHECK("signal frame semaphore", vkSignalSemaphore(fo->Device, &sem_sig_info));
	}

	return fo;
}

uint8_t FrameObjects_GetFrameInFlight(FrameObjects* fo)
{
	return fo->FrameInFlight;
}

VkCommandBuffer FrameObjects_GetCommandBuffer(FrameObjects* fo)
{
	return fo->CommandBuffers[fo->FrameInFlight];
}

VkSemaphore FrameObjects_GetSemaphore(FrameObjects* fo)
{
	return fo->FrameSemaphores[fo->FrameInFlight];
}

uint64_t& FrameObjects_GetFrameSemValue(FrameObjects* fo)
{
	return fo->FrameSemaphoreValues[fo->FrameInFlight];
}

void FrameObjects_NextFrame(FrameObjects* fo)
{
	fo->FrameInFlight = (fo->FrameInFlight + 1) % fo->MaxFramesInFlight;
}

void FrameObjects_Destroy(FrameObjects* fo)
{
	if (fo->Device != VK_NULL_HANDLE)
	{
		for (uint8_t fr = 0; fr < fo->MaxFramesInFlight; ++fr)
		{
			vkDestroySemaphore(fo->Device, fo->FrameSemaphores[fr], nullptr);
		}
		vkDestroyCommandPool(fo->Device, fo->CommandPool, nullptr);
	}

	free(fo);
}
