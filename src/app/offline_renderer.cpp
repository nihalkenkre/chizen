#include "offline_renderer.hpp"
#include <iostream>

void offline_renderer::render(const size_t render_width, const size_t render_height, const std::string& file_path, const void* cam_xform, uint8_t* pixels)
{
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    if (cgltf_parse_file(&options, file_path.c_str(), &data) != cgltf_result_success ||
        cgltf_validate(data) != cgltf_result_success ||
        cgltf_load_buffers(&options, data, file_path.c_str()) != cgltf_result_success)
    {
        std::cerr << "ERR Could not parse gltf file\n";
    }

    size_t render_data_size = render_width * render_height * 4;
    VkInstance instance = vk_instance::create();
    vk_phydev::data phy_dev = vk_phydev::get_phy_dev(instance);
    VkDevice device = vk_device::create(phy_dev.phy_dev, phy_dev.q_fly_idx, phy_dev.q_count);
    vk_command_pool::data cmd_pool = vk_command_pool::create(device, phy_dev.q_fly_idx, 10, "general command pool");
    VkFence rndr_fnc = vk_fence::create(device, 0, "render fence");

    VkQueue gfx_q = VK_NULL_HANDLE;
    VkQueue xfer_q = VK_NULL_HANDLE;
    VkDeviceQueueInfo2 q_info = {};
    q_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
    q_info.queueFamilyIndex = phy_dev.q_fly_idx;
    q_info.queueIndex = 0;
    vkGetDeviceQueue2(device, &q_info, &gfx_q);
    q_info.queueIndex = 1;
    vkGetDeviceQueue2(device, &q_info, &xfer_q);

    host_buffer_memory::data final_image_buffer_memory = host_buffer_memory::create(device, phy_dev.mem_props, render_data_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, "final image buffer");

    VkImage render_image = vk_image::create(device, { static_cast<uint32_t>(render_width), static_cast<uint32_t>(render_height), 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, "render image");

    VkImageMemoryRequirementsInfo2 mem_req_info = {};
    mem_req_info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
    mem_req_info.image = render_image;
    VkMemoryRequirements2 render_image_mem_reqs = {};
    render_image_mem_reqs.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
    vkGetImageMemoryRequirements2(device, &mem_req_info, &render_image_mem_reqs);

    VkDeviceMemory render_image_device_memory = vk_device_memory::allocate(device, render_image_mem_reqs.memoryRequirements.size, get_memory_type_id(phy_dev.mem_props, render_image_mem_reqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), "render image memory");

    VkBindImageMemoryInfo render_image_bind_info = {};
    render_image_bind_info.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
    render_image_bind_info.image = render_image;
    render_image_bind_info.memory = render_image_device_memory;

    const VkBindImageMemoryInfo bind_infos[] = {
        render_image_bind_info,
    };

    VK_CHECK("binding render image to memory", vkBindImageMemory2(device, _countof(bind_infos), bind_infos));

    host_buffer_memory::data staging_buffer_memory = host_buffer_memory::create(device, phy_dev.mem_props, render_data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, "render image staging buffer");

    for (size_t i = 0; i < render_data_size; i += 4)
    {
        reinterpret_cast<uint8_t*>(staging_buffer_memory.map)[i] = std::rand() % 255;
        reinterpret_cast<uint8_t*>(staging_buffer_memory.map)[i + 1] = std::rand() % 255;
        reinterpret_cast<uint8_t*>(staging_buffer_memory.map)[i + 2] = std::rand() % 255;
    }

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK("begin command buffer", vkBeginCommandBuffer(cmd_pool.cmd_buffs[0], &begin_info));

    VkImageMemoryBarrier2 img_mem_barr_1 = {};
    img_mem_barr_1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    img_mem_barr_1.image = render_image;
    img_mem_barr_1.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img_mem_barr_1.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    img_mem_barr_1.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    img_mem_barr_1.srcAccessMask = 0;
    img_mem_barr_1.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    img_mem_barr_1.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    img_mem_barr_1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_mem_barr_1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_mem_barr_1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    img_mem_barr_1.subresourceRange.layerCount = 1;
    img_mem_barr_1.subresourceRange.levelCount = 1;

    const VkImageMemoryBarrier2 img_mem_barrs_1[] = {
        img_mem_barr_1,
    };

    VkDependencyInfo dep_info = {};
    dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep_info.imageMemoryBarrierCount = _countof(img_mem_barrs_1);
    dep_info.pImageMemoryBarriers = img_mem_barrs_1;

    vkCmdPipelineBarrier2(cmd_pool.cmd_buffs[0], &dep_info);

    VkBufferImageCopy2 region = {};
    region.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
    region.imageExtent = { static_cast<uint32_t>(render_width), static_cast<uint32_t>(render_height), 1 };
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;

    const VkBufferImageCopy2 regions[] = {
        region,
    };

    VkCopyBufferToImageInfo2 buff_img_cpy_info = {};
    buff_img_cpy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
    buff_img_cpy_info.regionCount = _countof(regions);
    buff_img_cpy_info.pRegions = regions;
    buff_img_cpy_info.srcBuffer = staging_buffer_memory.buffer;
    buff_img_cpy_info.dstImage = render_image;
    buff_img_cpy_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    vkCmdCopyBufferToImage2(cmd_pool.cmd_buffs[0], &buff_img_cpy_info);

    VkImageMemoryBarrier2 img_mem_barr_2 = {};
    img_mem_barr_2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    img_mem_barr_2.image = render_image;
    img_mem_barr_2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    img_mem_barr_2.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    img_mem_barr_2.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    img_mem_barr_2.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    img_mem_barr_2.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    img_mem_barr_2.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
    img_mem_barr_2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_mem_barr_2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_mem_barr_2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    img_mem_barr_2.subresourceRange.layerCount = 1;
    img_mem_barr_2.subresourceRange.levelCount = 1;

    const VkImageMemoryBarrier2 img_mem_barrs_2[] = {
        img_mem_barr_2,
    };

    dep_info.imageMemoryBarrierCount = _countof(img_mem_barrs_2);
    dep_info.pImageMemoryBarriers = img_mem_barrs_2;

    vkCmdPipelineBarrier2(cmd_pool.cmd_buffs[0], &dep_info);

    VkCopyImageToBufferInfo2 img_buff_cpy_info = {};
    img_buff_cpy_info.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2;
    img_buff_cpy_info.regionCount = _countof(regions);
    img_buff_cpy_info.pRegions = regions;
    img_buff_cpy_info.srcImage = render_image;
    img_buff_cpy_info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    img_buff_cpy_info.dstBuffer = final_image_buffer_memory.buffer;

    vkCmdCopyImageToBuffer2(cmd_pool.cmd_buffs[0], &img_buff_cpy_info);

    VK_CHECK("end command buffer", vkEndCommandBuffer(cmd_pool.cmd_buffs[0]));

    VkCommandBufferSubmitInfo cmd_buff_info = {};
    cmd_buff_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmd_buff_info.commandBuffer = cmd_pool.cmd_buffs[0];

    const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
        cmd_buff_info,
    };

    VkSubmitInfo2 submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit_info.commandBufferInfoCount = _countof(cmd_buff_infos);
    submit_info.pCommandBufferInfos = cmd_buff_infos;

    const VkSubmitInfo2 submit_infos[] = {
        submit_info,
    };

    VK_CHECK("submit queue", vkQueueSubmit2(xfer_q, _countof(submit_infos), submit_infos, rndr_fnc));

    const VkFence wait_fences[] = {
        rndr_fnc,
    };

    VK_CHECK("wait for rndr fnc", vkWaitForFences(device, _countof(wait_fences), wait_fences, VK_TRUE, UINT64_MAX));
    VK_CHECK("reset rndr fnc", vkResetFences(device, _countof(wait_fences), wait_fences));

    std::memcpy(pixels, final_image_buffer_memory.map, render_data_size);

    vk_fence::destroy(rndr_fnc, device);
    vk_command_pool::destroy(cmd_pool, device);
    host_buffer_memory::destroy(staging_buffer_memory, device);
    host_buffer_memory::destroy(final_image_buffer_memory, device);
    vk_device_memory::free(render_image_device_memory, device);
    vk_image::destroy(render_image, device);

    vk_device::destroy(device);
    vk_instance::destroy(instance);
}
