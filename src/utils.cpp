#include "utils.hpp"

void Utils_ChangeImageLayout(
	const VkCommandBuffer cmd_buff,
	const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask,
	const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask,
	const VkImageLayout old_layout, const VkImageLayout new_layout,
	const uint32_t src_q_fly_idx, const uint32_t dst_q_fly_idx,
	const VkImageAspectFlags aspect_mask,
	const VkImage& image)
{
	const VkImageMemoryBarrier2 img_mem_barr = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = src_stage_mask,
		.srcAccessMask = src_access_mask,
		.dstStageMask = dst_stage_mask,
		.dstAccessMask = dst_access_mask,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = src_q_fly_idx,
		.dstQueueFamilyIndex = dst_q_fly_idx,
		.image = image,
		.subresourceRange = {
			.aspectMask = aspect_mask,
			.levelCount = 1,
			.layerCount = 1,
		},
	};

	const VkDependencyInfo dep_info = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &img_mem_barr,
	};

	vkCmdPipelineBarrier2KHR(cmd_buff, &dep_info);
}

void Utils_InsertMemoryBarrier(
	const VkCommandBuffer cmd_buff,
	const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask,
	const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask
)
{
	VkMemoryBarrier2 mem_bar = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = src_stage_mask,
		.srcAccessMask = src_access_mask,
		.dstStageMask = dst_stage_mask,
		.dstAccessMask = dst_access_mask,
	};

	const VkDependencyInfo dep_info = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &mem_bar,
	};

	vkCmdPipelineBarrier2KHR(cmd_buff, &dep_info);
}

void Utils_SetObjectName(const VkDevice device, const VkObjectType type, const uint64_t handle, const std::string& name)
{
	const VkDebugUtilsObjectNameInfoEXT name_info = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.objectType = type,
		.objectHandle = handle,
		.pObjectName = name.c_str(),
	};

	VK_CHECK("setting name", vkSetDebugUtilsObjectNameEXT(device, &name_info));
}

glm::mat4 Utils_GetTransformForGLTFNode(const cgltf_node* node)
{
	glm::mat4 xform = glm::mat4(1.f);

	if (node == nullptr)
		return xform;

	if (node->has_matrix)
	{
		xform = glm::make_mat4(node->matrix);
	}
	else
	{
		if (node->has_translation)
		{
			xform = glm::translate(xform, glm::make_vec3(node->translation));
		}

		if (node->has_rotation)
		{
			auto rot_quat = glm::make_quat(node->rotation);
			xform = glm::rotate(xform, glm::angle(rot_quat), glm::axis(rot_quat));
		}

		if (node->has_scale)
		{
			xform = glm::scale(xform, glm::make_vec3(node->scale));
		}
	}

	xform *= Utils_GetTransformForGLTFNode(node->parent);

	return xform;
}

