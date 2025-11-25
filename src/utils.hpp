#pragma once

#define VK_CHECK(action, result)						\
	if (result != VK_SUCCESS)							\
	{															\
		std::printf("%s %d\n", action, result);	\
	}

#define SDL_CHECK(result)						\
	if (!result) {									\
		SDL_Log("%s\n", SDL_GetError());		\
	}

#define SLANG_CHECK(action, result)					\
	if (result != SLANG_OK)								\
{																\
	std::printf("%s %d\n", action, result);		\
}

struct ImGUIState
{
	int TempRenderTargetExtent[2] = { 1280, 720 };
	int TempMaxSamples = 1024;
	bool StartRaytracing = false;
};

inline static VkDeviceSize ALIGNED_SIZE(VkDeviceSize size, VkDeviceSize alignment)
{
	return (size + alignment - 1) & ~(alignment - 1);
}

void Utils_CopyBufferToBuffer(const VkCommandBuffer xfer_cmd_buff, const VkQueue xfer_q, const VkBuffer src_buffer, const VkBuffer dst_buffer, const VkDeviceSize size);

void Utils_ChangeImageLayout(
	const VkCommandBuffer cmd_buff,
	const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask,
	const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask,
	const VkImageLayout old_layout, const VkImageLayout new_layout,
	const uint32_t src_q_fly_idx, const uint32_t dst_q_fly_idx,
	const VkImage& image);

void Utils_InitializeImages(
	const std::vector<VkImage> images,
	const VkCommandBuffer cmd_buff,
	const VkQueue queue
);

void Utils_InsertMemoryBarrier(
	const VkCommandBuffer cmd_buff,
	const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask,
	const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask
);

