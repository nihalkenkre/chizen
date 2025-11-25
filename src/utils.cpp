#include "utils.hpp"

void Utils_CopyBufferToBuffer(const VkCommandBuffer cmd_buff, const VkQueue queue, const VkBuffer src_buffer, const VkBuffer dst_buffer, const VkDeviceSize size)
{
	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VK_CHECK("begin xfer cmd buff", vkBeginCommandBuffer(cmd_buff, &begin_info));

	const VkBufferCopy2 regions[] = {
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
			.size = size,
		},
	};

	const VkCopyBufferInfo2 copy_buff_info = {
		.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
		.srcBuffer = src_buffer,
		.dstBuffer = dst_buffer,
		.regionCount = std::size(regions),
		.pRegions = regions,
	};

	vkCmdCopyBuffer2(cmd_buff, &copy_buff_info);
	VK_CHECK("end xfer cmd buff", vkEndCommandBuffer(cmd_buff));

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = cmd_buff,
		},
	};

	const VkSubmitInfo2 submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount = std::size(cmd_buff_infos),
			.pCommandBufferInfos = cmd_buff_infos,
		},
	};

	VK_CHECK("submit geom buffer xfer cmd", vkQueueSubmit2(queue, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));
	VK_CHECK("wait xfer cmd buff", vkQueueWaitIdle(queue));
}

void Utils_ChangeImageLayout(
	const VkCommandBuffer cmd_buff,
	const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask,
	const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask,
	const VkImageLayout old_layout, const VkImageLayout new_layout,
	const uint32_t src_q_fly_idx, const uint32_t dst_q_fly_idx,
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
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};

	const VkDependencyInfo dep_info = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &img_mem_barr,
	};

	vkCmdPipelineBarrier2(cmd_buff, &dep_info);
}

void Utils_InitializeImages(const std::vector<VkImage> images, const VkCommandBuffer cmd_buff, const VkQueue queue)
{
	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	VK_CHECK("begin cmd buff", vkBeginCommandBuffer(cmd_buff, &begin_info));

	for (const auto& image : images)
	{
		Utils_ChangeImageLayout(cmd_buff,
			VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
			VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			image);
	}

	VK_CHECK("end cmd buff", vkEndCommandBuffer(cmd_buff));

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = cmd_buff,
		},
	};

	const VkSubmitInfo2 submit_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.commandBufferInfoCount= std::size(cmd_buff_infos),
			.pCommandBufferInfos = cmd_buff_infos,
		},
	};

	VK_CHECK("submit tranfer commands", vkQueueSubmit2(queue, std::size(submit_infos), submit_infos, VK_NULL_HANDLE));
	VK_CHECK("queue wait idle", vkQueueWaitIdle(queue));
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

	vkCmdPipelineBarrier2(cmd_buff, &dep_info);
}

