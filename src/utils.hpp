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

uint32_t Utils_GetMemoryTypeId(const VkPhysicalDeviceMemoryProperties2 mem_props, const VkMemoryPropertyFlags mem_prop_flags);

inline static VkDeviceSize ALIGNED_SIZE(VkDeviceSize size, VkDeviceSize alignment)
{
	return (size + alignment - 1) & ~(alignment - 1);
}

void Utils_ChangeImageLayout(
	const VkCommandBuffer cmd_buff,
	const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask,
	const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask,
	const VkImageLayout old_layout, const VkImageLayout new_layout,
	const uint32_t src_q_fly_idx, const uint32_t dst_q_fly_idx,
	const VkImageAspectFlags aspect_mask,
	const VkImage& image);

void Utils_InsertMemoryBarrier(
	const VkCommandBuffer cmd_buff,
	const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask,
	const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask
);

void Utils_SetObjectName(const VkDevice device, const VkObjectType type, const uint64_t handle, const std::string& name);
glm::mat4 Utils_GetTransformForGLTFNode(const cgltf_node* node);
