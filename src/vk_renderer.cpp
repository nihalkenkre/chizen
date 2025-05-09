#include "vk_renderer.hpp"

#include <iostream>
#include <meshoptimizer/src/meshoptimizer.h>
#include <cglm/include/cglm/cglm.h>

#include <Shlwapi.h>

#include <SPIRV-Reflect/spirv_reflect.h>

#include <stb/stb_image.h>

constexpr uint8_t MAX_VERTICES = 64;
constexpr uint8_t MAX_TRIANGLES = 124;
constexpr float CAMERA_MOVEMENT_SPEED = 5.0;

inline static VkDeviceSize ALIGNED_SIZE(VkDeviceSize value, VkDeviceSize alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

inline static VkViewport RECT_TO_VIEWPORT(const RECT& rect)
{
    VkViewport v = {};
    v.x = 0;
    v.y = 0;
    v.width = static_cast<float>(rect.right - rect.left);
    v.height = static_cast<float>(rect.bottom - rect.top);
    v.minDepth = 0;
    v.maxDepth = 1;

    return v;
}

inline static RECT VIEWPORT_TO_RECT(const VkViewport& viewport)
{
    RECT r = {};
    r.left = static_cast<LONG>(viewport.x);
    r.top = static_cast<LONG>(viewport.y);
    r.right = static_cast<LONG>(viewport.width);
    r.bottom = static_cast<LONG>(viewport.height);

    return r;
}

inline static RECT SANITIZE_RECT_FOR_RENDER(const RECT& rect)
{
    RECT r = {};
    r.left = 0;
    r.top = 0;
    r.right = rect.right - rect.left;
    r.bottom = rect.bottom - rect.top;

    return r;
}

vk_renderer::vk_renderer(const HWND h_wnd) : img_idx(0), acq_wait_sem_val(0)
{
    instance = vk_instance::create();
    volkLoadInstance(instance);
    surface = vk_surface::create(instance, GetModuleHandleA(nullptr), h_wnd);
    phy_dev = vk_phy_dev::get_phy_dev(instance, &surface);
    device = vk_device::create(phy_dev.phy_dev, phy_dev.q_fly_idx, phy_dev.q_count);
    swapchain = vk_swapchain::create(device, surface, phy_dev, "swapchain");
    acq_sig_sem = vk_semaphore::create(device, VK_SEMAPHORE_TYPE_BINARY, "acquire signal semaphore");
    acq_wait_sem = vk_semaphore::create(device, VK_SEMAPHORE_TYPE_TIMELINE, "acquire wait semaphore");

    // signalling so that the render function does not stall on vkAcquireNextImage the first time
    VkSemaphoreSignalInfo sem_sig_info = {};
    sem_sig_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
    sem_sig_info.semaphore = acq_wait_sem.semaphore;
    sem_sig_info.value = ++acq_wait_sem_val;
    vkSignalSemaphore(device, &sem_sig_info);

    GetWindowRect(h_wnd, &wnd_rect);
    wnd_rect = SANITIZE_RECT_FOR_RENDER(wnd_rect);
    viewport = RECT_TO_VIEWPORT(wnd_rect);

    VkDeviceQueueInfo2 queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
    queue_info.queueFamilyIndex = phy_dev.q_fly_idx;
    queue_info.queueIndex = 0;
    vkGetDeviceQueue2(device, &queue_info, &gfx_q);

    queue_info.queueIndex = 1;
    vkGetDeviceQueue2(device, &queue_info, &xfer_q);

    xfer_cmd_pool = vk_command_pool::create(device, phy_dev.q_fly_idx, 1, "xfer command pool");
}

void vk_renderer::import_scene_data(const std::string& file_path)
{
    sd = scene_data::create(file_path, phy_dev, surface, swapchain.images_count, device, xfer_q, xfer_cmd_pool.cmd_buffs[0]);
}

void vk_renderer::handle_mouse_move(const POINT mouse_pos)
{
    i.mouse_pos = mouse_pos;
}

void vk_renderer::handle_mouse_l_btn_down()
{
    i.l_btn_down = true;
}

void vk_renderer::handle_mouse_l_btn_up()
{
    i.l_btn_down = false;
}

void vk_renderer::handle_mouse_m_btn_down()
{
    i.m_btn_down = true;
}

void vk_renderer::handle_mouse_m_btn_up()
{
    i.m_btn_down = false;
}

void vk_renderer::handle_mouse_r_btn_down()
{
    i.r_btn_down = true;
}

void vk_renderer::handle_mouse_r_btn_up()
{
    i.r_btn_down = false;
}

void vk_renderer::handle_w_down()
{
    i.w_down = true;
}

void vk_renderer::handle_a_down()
{
    i.a_down = true;
}

void vk_renderer::handle_s_down()
{
    i.s_down = true;
}

void vk_renderer::handle_d_down()
{
    i.d_down = true;
}

void vk_renderer::handle_q_down()
{
    i.q_down = true;
}

void vk_renderer::handle_e_down()
{
    i.e_down = true;
}

void vk_renderer::handle_w_up()
{
    i.w_down = false;
}

void vk_renderer::handle_a_up()
{
    i.a_down = false;
}

void vk_renderer::handle_s_up()
{
    i.s_down = false;
}

void vk_renderer::handle_d_up()
{
    i.d_down = false;
}

void vk_renderer::handle_q_up()
{
    i.q_down = false;
}

void vk_renderer::handle_e_up()
{
    i.e_down = false;
}

void vk_renderer::resize(const uint32_t width, const uint32_t height)
{
    VK_CHECK("wait for present fence", vkWaitForFences(device, 1, &swapchain.present_fences[img_idx], VK_TRUE, UINT64_MAX));
    VK_CHECK("reset present fence", vkResetFences(device, 1, &swapchain.present_fences[img_idx]));

    VK_CHECK("get surface capabilities", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phy_dev.phy_dev, surface.surface, &surface.surf_caps));

    vk_swapchain::destroy(swapchain, device);
    swapchain = vk_swapchain::create(device, surface, phy_dev, "swapchain");
}

void vk_renderer::begin_frame()
{
    VkSemaphoreWaitInfo wait_info = {};
    wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    wait_info.semaphoreCount = 1;
    wait_info.pSemaphores = &acq_wait_sem.semaphore;
    wait_info.pValues = &acq_wait_sem_val;
    vkWaitSemaphores(device, &wait_info, UINT64_MAX);

    VK_CHECK("acquire image index", vkAcquireNextImageKHR(device, swapchain.swapchain, UINT64_MAX, acq_sig_sem.semaphore, VK_NULL_HANDLE, &img_idx));
    VK_CHECK("reset command buffer", vkResetCommandBuffer(swapchain.cmd_buffs[img_idx], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

    VkImageMemoryBarrier2 img_mem_barr2 = {};
    img_mem_barr2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    img_mem_barr2.srcStageMask = 0;
    img_mem_barr2.srcAccessMask = 0;
    img_mem_barr2.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    img_mem_barr2.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    img_mem_barr2.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    img_mem_barr2.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    img_mem_barr2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_mem_barr2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_mem_barr2.image = swapchain.images[img_idx];
    img_mem_barr2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    img_mem_barr2.subresourceRange.levelCount = 1;
    img_mem_barr2.subresourceRange.layerCount = 1;

    VkDependencyInfo dep_info = {};
    dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers = &img_mem_barr2;

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(swapchain.cmd_buffs[img_idx], &begin_info);
    vkCmdPipelineBarrier2(swapchain.cmd_buffs[img_idx], &dep_info);
}

void vk_renderer::clear_frame(const float color[])
{
    std::vector<VkRenderingAttachmentInfoKHR> color_attachment_infos(1);
    color_attachment_infos[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color_attachment_infos[0].imageView = swapchain.image_views[img_idx];
    color_attachment_infos[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment_infos[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_infos[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_infos[0].clearValue.color.float32[0] = color[0];
    color_attachment_infos[0].clearValue.color.float32[1] = color[1];
    color_attachment_infos[0].clearValue.color.float32[2] = color[2];
    color_attachment_infos[0].clearValue.color.float32[3] = color[3];

    VkRenderingInfoKHR rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    rendering_info.renderArea.extent = surface.surf_caps.currentExtent;
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = static_cast<uint32_t>(color_attachment_infos.size());
    rendering_info.pColorAttachments = color_attachment_infos.data();

    vkCmdBeginRendering(swapchain.cmd_buffs[img_idx], &rendering_info);
}

void vk_renderer::render_world()
{
    std::vector<VkViewport>viewports(1);
    viewports[0].width = static_cast<float>(surface.surf_caps.currentExtent.width);
    viewports[0].height = static_cast<float>(surface.surf_caps.currentExtent.height);
    viewports[0].minDepth = 0;
    viewports[0].maxDepth = 1;

    std::vector<VkRect2D> scissors(1);
    scissors[0].extent = surface.surf_caps.currentExtent;

    vkCmdSetScissor(swapchain.cmd_buffs[img_idx], 0, static_cast<uint32_t>(scissors.size()), scissors.data());
    vkCmdSetPrimitiveTopology(swapchain.cmd_buffs[img_idx], VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    vkCmdSetPolygonModeEXT(swapchain.cmd_buffs[img_idx], VK_POLYGON_MODE_FILL);
    vkCmdSetViewport(swapchain.cmd_buffs[img_idx], 0, static_cast<uint32_t>(viewports.size()), viewports.data());

#ifdef DESC_BUFFER
    vkCmdBindPipeline(swapchain.cmd_buffs[img_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, vtx_pipeline_d_buff->pipeline);
    const VkDescriptorBufferBindingInfoEXT binding_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
            .address = sd.desc_buff_mem->addr,
            .usage = sd.desc_buff_mem->usage,
        },
    };

    vkCmdBindDescriptorBuffersEXT(swapchain.cmd_buffs[img_idx], _countof(binding_infos), binding_infos);

    const uint32_t buff_idxs[] = { 0 };
    VkDeviceSize desc_buff_offset = 0;

    for (auto const& mi : sd.mis)
    {
        for (auto const& pd : mi.pds)
        {
            vkCmdSetDescriptorBufferOffsetsEXT(swapchain.cmd_buffs[img_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, vtx_pipeline_d_buff->pipeline_layout, 0, _countof(buff_idxs), buff_idxs, &desc_buff_offset);

            const VkBuffer buffers[] = {
                sd.geom_buff_mem->buffer->buffer,
                sd.geom_buff_mem->buffer->buffer,
            };

            const VkDeviceSize offsets[] = {
                pd.vsi.positions_offset,
                pd.vsi.uvs_offsets,
            };

            vkCmdBindVertexBuffers(swapchain.cmd_buffs[img_idx], 0, _countof(buffers), buffers, offsets);
            vkCmdBindIndexBuffer(swapchain.cmd_buffs[img_idx], sd.geom_buff_mem->buffer->buffer, pd.vsi.indices_offset, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(swapchain.cmd_buffs[img_idx], pd.vsi.indices_count, 1, 0, 0, 0);
            desc_buff_offset += vtx_pipeline_d_buff->dsl_infos[0].aligned_size;
        }
    }

#else
    vkCmdBindPipeline(swapchain.cmd_buffs[img_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, sd.vtx_pipeline_d_sets.pipeline);

    std::vector<VkWriteDescriptorSet> write_desc_sets(1);
    write_desc_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_desc_sets[0].dstSet = sd.desc_set;
    write_desc_sets[0].dstBinding = 0;
    write_desc_sets[0].descriptorCount = 1;
    write_desc_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_desc_sets[0].pBufferInfo = &sd.cam.xform_descs[img_idx];

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(write_desc_sets.size()), write_desc_sets.data(), 0, nullptr);

    // set 0 scene level
    vkCmdBindDescriptorSets(swapchain.cmd_buffs[img_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, sd.vtx_pipeline_d_sets.pipeline_layout, 0, 1, &sd.desc_set, 0, nullptr);

    for (auto const& mi : sd.mis)
    {
        // set 1 material level
        vkCmdBindDescriptorSets(swapchain.cmd_buffs[img_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, sd.vtx_pipeline_d_sets.pipeline_layout, 1, 1, &mi.desc_set, 0, nullptr);

        for (auto const& pd : mi.pds)
        {
            // set 2 primitive level
            vkCmdBindDescriptorSets(swapchain.cmd_buffs[img_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, sd.vtx_pipeline_d_sets.pipeline_layout, 2, 1, &pd.desc_set, 0, nullptr);

            const VkBuffer buffers[] = {
                sd.geom_buff_mem.buffer,
                sd.geom_buff_mem.buffer,
                sd.geom_buff_mem.buffer,
            };

            const VkDeviceSize offsets[] = {
                pd.vsi.positions_offset,
                pd.vsi.uvs_offset,
                pd.vsi.nrms_offset,
            };

            vkCmdBindVertexBuffers(swapchain.cmd_buffs[img_idx], 0, _countof(buffers), buffers, offsets);
            vkCmdBindIndexBuffer(swapchain.cmd_buffs[img_idx], sd.geom_buff_mem.buffer, pd.vsi.indices_offset, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(swapchain.cmd_buffs[img_idx], pd.vsi.indices_count, 1, 0, 0, 0);
        }
    }
#endif // DESC_BUFFER
}

void vk_renderer::end_frame()
{
    vkCmdEndRendering(swapchain.cmd_buffs[img_idx]);

    VkImageMemoryBarrier2 img_mem_barr = {};
    img_mem_barr.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    img_mem_barr.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    img_mem_barr.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    img_mem_barr.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    img_mem_barr.dstAccessMask = 0;
    img_mem_barr.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    img_mem_barr.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    img_mem_barr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_mem_barr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    img_mem_barr.image = swapchain.images[img_idx];
    img_mem_barr.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    img_mem_barr.subresourceRange.levelCount = 1;
    img_mem_barr.subresourceRange.layerCount = 1;

    VkDependencyInfo dep_info = {};
    dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers = &img_mem_barr;

    vkCmdPipelineBarrier2(swapchain.cmd_buffs[img_idx], &dep_info);

    vkEndCommandBuffer(swapchain.cmd_buffs[img_idx]);

    std::vector<VkSemaphoreSubmitInfo> wait_sem_infos(1);
    wait_sem_infos[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    wait_sem_infos[0].semaphore = acq_sig_sem.semaphore;
    wait_sem_infos[0].stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;

    std::vector<VkSemaphoreSubmitInfo> sig_sem_infos(2);
    sig_sem_infos[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    sig_sem_infos[0].semaphore = swapchain.rndr_semaphores[img_idx];
    sig_sem_infos[0].stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    sig_sem_infos[1].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    sig_sem_infos[1].semaphore = acq_wait_sem.semaphore;
    sig_sem_infos[1].value = ++acq_wait_sem_val;
    sig_sem_infos[1].stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

    std::vector<VkCommandBufferSubmitInfo> cmd_buff_infos(1);
    cmd_buff_infos[0].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmd_buff_infos[0].commandBuffer = swapchain.cmd_buffs[img_idx];

    VkSubmitInfo2 submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit_info.waitSemaphoreInfoCount = static_cast<uint32_t>(wait_sem_infos.size());
    submit_info.pWaitSemaphoreInfos = wait_sem_infos.data();
    submit_info.commandBufferInfoCount = static_cast<uint32_t>(cmd_buff_infos.size());
    submit_info.pCommandBufferInfos = cmd_buff_infos.data();
    submit_info.signalSemaphoreInfoCount = static_cast<uint32_t>(sig_sem_infos.size());
    submit_info.pSignalSemaphoreInfos = sig_sem_infos.data();

    vkQueueSubmit2(gfx_q, 1, &submit_info, VK_NULL_HANDLE);

    VkSwapchainPresentFenceInfoEXT present_fence_info = {};
    present_fence_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT;
    present_fence_info.swapchainCount = 1;
    present_fence_info.pFences = &swapchain.present_fences[img_idx];

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = &present_fence_info;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &swapchain.rndr_semaphores[img_idx];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain.swapchain;
    present_info.pImageIndices = &img_idx;

    VK_CHECK("reset present fence", vkResetFences(device, 1, &swapchain.present_fences[img_idx]));
    vkQueuePresentKHR(gfx_q, &present_info);
}

void vk_renderer::clear_scene_data()
{
    VK_CHECK("wait for present fence", vkWaitForFences(device, 1, &swapchain.present_fences[img_idx], VK_TRUE, UINT64_MAX));
    VK_CHECK("reset present fence", vkResetFences(device, 1, &swapchain.present_fences[img_idx]));

    scene_data::destroy(sd, device);
}

vk_renderer::~vk_renderer()
{
    VK_CHECK("wait for present fence", vkWaitForFences(device, 1, &swapchain.present_fences[img_idx], VK_TRUE, UINT64_MAX));
    VK_CHECK("reset present fence", vkResetFences(device, 1, &swapchain.present_fences[img_idx]));

    scene_data::destroy(sd, device);
    vk_semaphore::destroy(acq_sig_sem.semaphore, device);
    vk_semaphore::destroy(acq_wait_sem.semaphore, device);
    vk_command_pool::destroy(xfer_cmd_pool.cmd_pool, device);
    vk_swapchain::destroy(swapchain, device);
    vk_device::destroy(device);
    vk_surface::destroy(surface.surface, instance);
    vk_instance::destroy(instance);
}

scene_data::data scene_data::create(const std::string& file_path, const vk_phy_dev::data& phy_dev, const vk_surface::data& surface, const uint32_t swapchain_images_count, const VkDevice& device, const VkQueue& xfer_q, const VkCommandBuffer& cmd_buff)
{
    data d;

    char curr_dir[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA(NULL), curr_dir, MAX_PATH);

    PathRemoveFileSpecA(curr_dir);

    std::string raster_vtx_path = std::string(curr_dir).append("\\shaders\\pbr\\raster\\vertex\\");
    d.vtx_pipeline_d_sets = vk_graphics_pipeline::create(device, raster_vtx_path, CHI_PIPELINE_TYPE::VERTEX, surface.format.format, phy_dev.desc_buff_props, "vtx pipeline d sets");

    //    for (auto const& file : std::filesystem::directory_iterator(std::filesystem::path(tmp)))
    //    {
    //        if (std::filesystem::is_directory(file))
    //        {
    //            for (auto const& pipeline_type : std::filesystem::directory_iterator(std::filesystem::path(file)))
    //            {
    //                if (std::filesystem::is_directory(pipeline_type)) {
    //
    //                    if (pipeline_type.path().string().find("mesh") != std::string::npos) {
    //                        d.mesh_pipeline_d_sets = vk_graphics_pipeline::create(device, pipeline_type.path().string(), CHI_PIPELINE_TYPE::MESH, surface.format.format, phy_dev.desc_buff_props, "mesh pipeline d sets");
    //#ifdef DESC_BUFFER
    //                        d.mesh_d_buff_pipeline = vk_graphics_pipeline::create(device, pipeline_type.path().string(), CHI_PIPELINE_TYPE::MESH, surface.format.format, phy_dev.desc_buff_props);
    //#endif // DESC_BUFFER
    //                    }
    //                    else if (pipeline_type.path().string().find("vertex") != std::string::npos) {
    //                        d.vtx_pipeline_d_sets = vk_graphics_pipeline::create(device, pipeline_type.path().string(), CHI_PIPELINE_TYPE::VERTEX, surface.format.format, phy_dev.desc_buff_props, "vtx pipeline d sets");
    //#ifdef DESC_BUFFER
    //                        d.vtx_pipeline_d_buff = vk_graphics_pipeline::create(device, pipeline_type.path().string(), CHI_PIPELINE_TYPE::VERTEX, surface.format.format, phy_dev.desc_buff_props);
    //#endif // DESC_BUFFER
    //                    }
    //                }
    //            }
    //        }
    //    }

    d.cam_xform_d_buff_desc.resize(phy_dev.desc_buff_props.uniformBufferDescriptorSize);

    cgltf_options options = {};
    cgltf_data* data = nullptr;

    if (cgltf_parse_file(&options, file_path.c_str(), &data) != cgltf_result_success ||
        cgltf_validate(data) != cgltf_result_success ||
        cgltf_load_buffers(&options, data, file_path.c_str()) != cgltf_result_success)
    {
        std::cerr << "ERR Could not parse gltf file\n";
        return d;
    }

    // Meshlets data (positions, meshlets, vertices, indices)
    std::vector<uint8_t> mesh_geom_data;

    // Vertex, index data
    std::vector<uint8_t> vertex_geom_data;
    // Xform data for camera and primitives
    std::vector<uint8_t> xform_data;
    // images data
    std::vector<uint8_t> images_data;

    VkDeviceSize aligned_mat_size = ALIGNED_SIZE(sizeof(mat4), phy_dev.props.properties.limits.minUniformBufferOffsetAlignment);

    xform_data.resize(aligned_mat_size * swapchain_images_count); // viewport "camera", one xform for each swapchain image

    for (cgltf_size n = 0; n < data->nodes_count; ++n)
    {
        cgltf_node* curr_node = data->nodes + n;

        if (curr_node->mesh == nullptr)
            continue;

        cgltf_mesh* curr_mesh = curr_node->mesh;

        for (cgltf_size p = 0; p < curr_mesh->primitives_count; ++p)
        {
            mat4 xform;
            glm_mat4_identity(xform);

            if (curr_node->has_matrix)
            {
                std::memcpy(xform, curr_node->matrix, sizeof(xform));
            }
            else {
                if (curr_node->has_scale)
                {
                    glm_scale(xform, curr_node->scale);
                }

                if (curr_node->has_rotation)
                {
                    glm_quat_rotate(xform, curr_node->rotation, xform);
                }

                if (curr_node->has_translation)
                {
                    glm_translate(xform, curr_node->translation);
                }
            }

            VkDeviceSize curr_xform_data_size = xform_data.size();

            xform_data.resize(xform_data.size() + aligned_mat_size);
            std::memcpy(xform_data.data() + curr_xform_data_size, xform, sizeof(xform));

            std::vector<uint32_t> indices;
            std::vector<uint8_t> pos;
            std::vector<uint8_t> uvs;
            std::vector<uint8_t> nrms;

            cgltf_primitive* curr_prim = curr_mesh->primitives + p;
            if (curr_prim->material == nullptr)
                continue;

            if (curr_prim->indices->component_type == cgltf_component_type_r_32u)
            {
                size_t curr_ind_size = indices.size();
                indices.resize(indices.size() + curr_prim->indices->count);
                std::memcpy(indices.data() + curr_ind_size, (void*)((ULONG_PTR)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->offset + curr_prim->indices->buffer_view->offset), curr_prim->indices->buffer_view->size);
            }
            else if (curr_prim->indices->component_type == cgltf_component_type_r_16u)
            {
                indices.reserve(indices.size() + curr_prim->indices->count);
                uint16_t* idx_ptr = (uint16_t*)((ULONG_PTR)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->offset + curr_prim->indices->buffer_view->offset);

                for (cgltf_size i = 0; i < curr_prim->indices->count; ++i)
                {
                    indices.push_back(idx_ptr[i]);
                }
            }

            primitive_data pd;
            pd.vsi.indices_count = static_cast<uint32_t>(indices.size());

            for (cgltf_size a = 0; a < curr_prim->attributes_count; ++a)
            {
                cgltf_attribute* curr_attr = curr_prim->attributes + a;

                if (std::strcmp(curr_attr->name, "POSITION") == 0)
                {
                    size_t curr_pos_size = pos.size();
                    pos.resize(pos.size() + (curr_attr->data->count * curr_attr->data->stride));
                    std::memcpy(&pos[curr_pos_size], (void*)((ULONG_PTR)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->count * curr_attr->data->stride);
                }
                else if (std::strcmp(curr_attr->name, "TEXCOORD_0") == 0)
                {
                    size_t curr_uvs_size = uvs.size();
                    uvs.resize(uvs.size() + (curr_attr->data->count * curr_attr->data->stride));
                    std::memcpy(&uvs[curr_uvs_size], (void*)((ULONG_PTR)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->count * curr_attr->data->stride);
                }
                else if (std::strcmp(curr_attr->name, "NORMAL") == 0)
                {
                    size_t curr_nrm_size = nrms.size();
                    nrms.resize(nrms.size() + (curr_attr->data->count * curr_attr->data->stride));
                    std::memcpy(&nrms[curr_nrm_size], (void*)((ULONG_PTR)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->count * curr_attr->data->stride);
                }
            }

            // details for mesh pipeline
           /* {
                size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), MAX_VERTICES, MAX_TRIANGLES);

                std::vector<meshopt_Meshlet> meshlets(max_meshlets);
                std::vector<uint32_t>meshlets_vertices(max_meshlets * MAX_VERTICES);
                std::vector<uint8_t>meshlets_triangles(max_meshlets * MAX_TRIANGLES);

                pd.msi.meshlets_count = meshopt_buildMeshlets(
                    meshlets.data(),
                    meshlets_vertices.data(),
                    meshlets_triangles.data(),
                    indices.data(),
                    indices.size(),
                    reinterpret_cast<float*>(positions.data()),
                    positions.size(),
                    sizeof(float3),
                    MAX_VERTICES,
                    MAX_TRIANGLES,
                    0.0
                );

                meshopt_Meshlet last_meshlet = meshlets[pd.msi.meshlets_count - 1];
                meshlets_vertices.resize(last_meshlet.vertex_offset + last_meshlet.vertex_count);
                meshlets_triangles.resize(last_meshlet.triangle_offset + ((last_meshlet.triangle_count * 3 + 3) & ~3));
                meshlets.resize(pd.msi.meshlets_count);

                std::vector<uint32_t> meshlet_triangles_32;
                size_t t_32_idx = 0;

                for (auto& meshlet : meshlets)
                {
                    uint32_t triangle_offset = static_cast<uint32_t>(meshlet_triangles_32.size());

                    for (size_t t = 0; t < meshlet.triangle_count; ++t)
                    {
                        uint32_t curr_tri = (static_cast<uint32_t>(meshlets_triangles[meshlet.triangle_offset + (t * 3)]) << 0) |
                            (static_cast<uint32_t>(meshlets_triangles[meshlet.triangle_offset + (t * 3 + 1)]) << 8) |
                            (static_cast<uint32_t>(meshlets_triangles[meshlet.triangle_offset + (t * 3 + 2)]) << 16);

                        meshlet_triangles_32.push_back(curr_tri);
                    };

                    meshlet.triangle_offset = triangle_offset;
                }

                //pd.msi.geom_descs[0].offset = geom_data.size();
                size_t positions_data_size = ALIGNED_SIZE(positions.size() * sizeof(positions[0]), phy_dev->props.properties.limits.minStorageBufferOffsetAlignment);
                //pd.msi.geom_descs[0].range = positions_data_size;
                std::vector<uint8_t> positions_data(positions_data_size);
                std::memcpy(positions_data.data(), positions.data(), positions_data_size);
                pd.msi.geom_data_offsets[0] = mesh_geom_data.size();
                mesh_geom_data.insert(mesh_geom_data.end(), positions_data.begin(), positions_data.end());
                pd.msi.geom_data_sizes[0] = positions_data_size;

                //pd.msi.geom_descs[1].offset = geom_data.size();
                size_t meshlets_data_size = ALIGNED_SIZE(meshlets.size() * sizeof(meshlets[0]), phy_dev->props.properties.limits.minStorageBufferOffsetAlignment);
                //pd.msi.geom_descs[1].range = meshlets_data_size;
                std::vector<uint8_t> meshlets_data(meshlets_data_size);
                std::memcpy(meshlets_data.data(), meshlets.data(), meshlets_data_size);
                pd.msi.geom_data_offsets[1] = mesh_geom_data.size();
                mesh_geom_data.insert(mesh_geom_data.end(), meshlets_data.begin(), meshlets_data.end());
                pd.msi.geom_data_sizes[1] = meshlets_data_size;

                //pd.msi.geom_descs[2].offset = geom_data.size();
                size_t meshlets_vertices_data_size = ALIGNED_SIZE(meshlets_vertices.size() * sizeof(meshlets_vertices[0]), phy_dev->props.properties.limits.minStorageBufferOffsetAlignment);
                //pd.msi.geom_descs[2].range = meshlets_vertices_data_size;
                std::vector<uint8_t> meshlets_vertices_data(meshlets_vertices_data_size);
                std::memcpy(meshlets_vertices_data.data(), meshlets_vertices.data(), meshlets_vertices_data_size);
                pd.msi.geom_data_offsets[2] = mesh_geom_data.size();
                mesh_geom_data.insert(mesh_geom_data.end(), meshlets_vertices.begin(), meshlets_vertices.end());
                pd.msi.geom_data_sizes[2] = meshlets_vertices_data_size;

                //pd.msi.geom_descs[3].offset = geom_data.size();
                size_t meshlets_triangles_data_size = ALIGNED_SIZE(meshlet_triangles_32.size() * sizeof(meshlet_triangles_32[0]), phy_dev->props.properties.limits.minStorageBufferOffsetAlignment);
                //pd.msi.geom_descs[3].range = meshlets_triangles_data_size;
                std::vector<uint8_t> meshlets_triangles_data(meshlets_triangles_data_size);
                std::memcpy(meshlets_triangles_data.data(), meshlet_triangles_32.data(), meshlets_triangles_data_size);
                pd.msi.geom_data_offsets[3] = mesh_geom_data.size();
                mesh_geom_data.insert(mesh_geom_data.end(), meshlets_triangles_data.begin(), meshlets_triangles_data.end());
                pd.msi.geom_data_sizes[3] = meshlets_triangles_data_size;
            }*/

            // details for vertex pipeline
            {
                // copy indices data
                size_t vertex_geom_data_size = vertex_geom_data.size();
                pd.vsi.indices_offset = vertex_geom_data_size;

                vertex_geom_data.resize(vertex_geom_data.size() + indices.size() * sizeof(indices[0]));

                std::memcpy(vertex_geom_data.data() + vertex_geom_data_size, indices.data(), indices.size() * sizeof(indices[0]));

                // copy positions data
                vertex_geom_data_size = vertex_geom_data.size();
                pd.vsi.positions_offset = vertex_geom_data_size;

                vertex_geom_data.resize(vertex_geom_data.size() + pos.size());
                std::memcpy(vertex_geom_data.data() + vertex_geom_data_size, pos.data(), pos.size());

                // copy uvs data
                vertex_geom_data_size = vertex_geom_data.size();
                pd.vsi.uvs_offset = vertex_geom_data_size;

                vertex_geom_data.resize(vertex_geom_data.size() + uvs.size());
                std::memcpy(vertex_geom_data.data() + vertex_geom_data_size, uvs.data(), uvs.size());

                // copy nrms data
                vertex_geom_data_size = vertex_geom_data.size();
                pd.vsi.nrms_offset = vertex_geom_data_size;

                vertex_geom_data.resize(vertex_geom_data.size() + nrms.size());
                std::memcpy(vertex_geom_data.data() + vertex_geom_data_size, nrms.data(), nrms.size());
            }

            cgltf_material* mat = curr_prim->material;

            material_descriptors mat_dscs = {};

            if (mat->has_pbr_metallic_roughness)
            {
                metal_rough_descriptors mr_dscs = {};
                mr_dscs.base_color_desc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                mr_dscs.base_color_factors[0] = mat->pbr_metallic_roughness.base_color_factor[0];
                mr_dscs.base_color_factors[1] = mat->pbr_metallic_roughness.base_color_factor[1];
                mr_dscs.base_color_factors[2] = mat->pbr_metallic_roughness.base_color_factor[2];
                mr_dscs.base_color_factors[3] = mat->pbr_metallic_roughness.base_color_factor[3];
                mr_dscs.metalness_factor = mat->pbr_metallic_roughness.metallic_factor;
                mr_dscs.roughness_factor = mat->pbr_metallic_roughness.roughness_factor;

                if (mat->pbr_metallic_roughness.base_color_texture.texture != nullptr)
                {
                    cgltf_texture* tex = mat->pbr_metallic_roughness.base_color_texture.texture;
                    if (tex->image != nullptr)
                    {
                        cgltf_image* img = tex->image;
                        if (img->uri != nullptr)
                        {

                        }
                        else
                        {
                            mr_dscs.base_color_pixels = stbi_load_from_memory(reinterpret_cast<stbi_uc*>(reinterpret_cast<uint64_t>(img->buffer_view->buffer->data) + img->buffer_view->offset), static_cast<int>(img->buffer_view->size), &mr_dscs.base_color_width, &mr_dscs.base_color_height, nullptr, 4);
                            mr_dscs.base_color_len = static_cast<VkDeviceSize>(mr_dscs.base_color_width) * static_cast<VkDeviceSize>(mr_dscs.base_color_height) * 4;
                        }

                        VkExtent3D extent = {};
                        extent.width = static_cast<uint32_t>(mr_dscs.base_color_width);
                        extent.height = static_cast<uint32_t>(mr_dscs.base_color_height);
                        extent.depth = 1;
                        mr_dscs.base_color_image = vk_image::create(device,
                            extent,
                            VK_FORMAT_R8G8B8A8_UNORM,
                            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                            "base color texture image"
                        );

                        vkGetImageMemoryRequirements(device, mr_dscs.base_color_image, &mr_dscs.base_color_mem_reqs);

                        size_t image_data_current_size = images_data.size();
                        mr_dscs.base_color_pixels_offset = image_data_current_size;
                        images_data.resize(image_data_current_size + mr_dscs.base_color_mem_reqs.size);
                        std::memcpy(images_data.data() + image_data_current_size, mr_dscs.base_color_pixels, mr_dscs.base_color_len);
                    }

                    if (tex->sampler != nullptr)
                    {
                        mr_dscs.base_color_desc.sampler = vk_sampler::create(device, tex->sampler->min_filter, tex->sampler->mag_filter, tex->sampler->wrap_s, tex->sampler->wrap_t, phy_dev.props.properties.limits.maxSamplerAnisotropy, 0.0, 0.0, "base color sampler");
                    }
                }

                mat_dscs.met_rough_dscs = mr_dscs;
            }
            else
            {
            }

            uint64_t mat_id = std::hash<std::string>{}(curr_prim->material->name);
            auto it = std::find_if(d.mis.begin(), d.mis.end(), [mat_id](const material_info& mi) { return mi.id == mat_id; });

            if (it == d.mis.end())
            {
                material_info mi = {};
                mi.id = mat_id;
                mi.mat_dscs = mat_dscs;
                mi.pds = { pd };

                d.mis.push_back(mi);
            }
            else
            {
                it->pds.push_back(std::move(pd));
            }
        }
    }

    cgltf_free(data);

    host_buffer_memory::data images_staging = host_buffer_memory::create(device, phy_dev.mem_props, 0, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, images_data, "images staging buffer");

    // nasty hack. the format of the images are the same, and thus the memory type id would also be the same, so just get the mem_reqs of the first one and allocate.
    d.images_memory = vk_device_memory::allocate(device, images_data.size(), get_memory_type_id(phy_dev.mem_props, d.mis[0].mat_dscs.met_rough_dscs.base_color_mem_reqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), "images device memory");

    for (auto& mi : d.mis)
    {
        VK_CHECK("bind image to memory", vkBindImageMemory(device, mi.mat_dscs.met_rough_dscs.base_color_image, d.images_memory, mi.mat_dscs.met_rough_dscs.base_color_pixels_offset));

        mi.mat_dscs.met_rough_dscs.base_color_desc.imageView = vk_image_view::create(device, mi.mat_dscs.met_rough_dscs.base_color_image, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, "base color texture image view");

        std::vector<VkBufferImageCopy2> regions(1);
        regions[0].sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
        regions[0].bufferOffset = mi.mat_dscs.met_rough_dscs.base_color_pixels_offset;
        regions[0].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        regions[0].imageSubresource.layerCount = 1;
        regions[0].imageExtent.width = static_cast<uint32_t>(mi.mat_dscs.met_rough_dscs.base_color_width);
        regions[0].imageExtent.height = static_cast<uint32_t>(mi.mat_dscs.met_rough_dscs.base_color_height);
        regions[0].imageExtent.depth = 1;

        copy_buffer_to_image(images_staging.buffer, mi.mat_dscs.met_rough_dscs.base_color_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions, cmd_buff, xfer_q);
    }

    host_buffer_memory::destroy(images_staging, device);

    d.geom_buff_mem = device_buffer_memory::create(
        device, phy_dev.mem_props, 0,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        vertex_geom_data, xfer_q, cmd_buff, "geom buffer memory");
    d.uni_buff_mem = host_buffer_memory::create(
        device, phy_dev.mem_props, 0,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        xform_data, "uniform buffer memory");

    mat4 identity_mat;
    glm_mat4_identity(identity_mat);

    // initial viewport camera xform
    mat4 v;
    vec3 eye = { 2, 5, -2 };
    vec3 center = { 0, 1, 0 };
    vec3 up = { 0, 1, 0 };
    glm_lookat(eye, center, up, v);

    mat4 p;
    glm_perspective(90.f, (float)surface.surf_caps.currentExtent.width / (float)surface.surf_caps.currentExtent.height, 0.1, 1000.f, p);
    p[1][1] *= -1;

    mat4 vp;
    glm_mul(p, v, vp);

    d.cam.xform = d.uni_buff_mem.map;
    std::memcpy(d.cam.xform, vp, sizeof(mat4));
    std::memcpy(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(d.cam.xform) + aligned_mat_size), vp, sizeof(mat4));

    // prepare scene level descs (camera xform descs)
    VkDeviceSize uni_buff_offset = 0;
    d.cam.xform_descs.resize(swapchain_images_count);

    for (uint32_t i = 0; i < swapchain_images_count; ++i)
    {
        d.cam.xform_descs[i].buffer = d.uni_buff_mem.buffer;
        d.cam.xform_descs[i].offset = uni_buff_offset;
        d.cam.xform_descs[i].range = sizeof(mat4);

        uni_buff_offset += aligned_mat_size;
    }

    // create scene level desc pool/set (camera xforms)
    std::vector<VkDescriptorPoolSize> pool_sizes(1);
    // camera xform descs
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = swapchain_images_count;

    d.desc_pool = vk_descriptor_pool::create(device, 1, pool_sizes, "cam xform descriptor pool");
    std::vector<VkDescriptorSetLayout> dsls(1, d.vtx_pipeline_d_sets.dsls[0]);
    d.desc_set = vk_descriptor_sets::allocate(device, d.desc_pool, dsls, "cam xform descriptor set ")[0];

    // material and primtive xform/texture descriptors

    for (auto& mi : d.mis)
    {
        // create texture desc pools/sets per material
        std::vector<VkDescriptorPoolSize> pool_sizes(1);
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[0].descriptorCount = 5;

        mi.desc_pool = vk_descriptor_pool::create(device, 1, pool_sizes, "per material textures desc pool");
        std::vector<VkDescriptorSetLayout> dsls(1, d.vtx_pipeline_d_sets.dsls[1]);
        mi.desc_set = vk_descriptor_sets::allocate(device, mi.desc_pool, dsls, "per material textures desc set ")[0];

        std::vector<VkWriteDescriptorSet> write_desc_sets(1);
        write_desc_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc_sets[0].dstSet = mi.desc_set;
        write_desc_sets[0].dstBinding = 0;
        write_desc_sets[0].descriptorCount = 1;
        write_desc_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_desc_sets[0].pImageInfo = &mi.mat_dscs.met_rough_dscs.base_color_desc;
        //{
        //    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        //    .dstSet = mi.desc_set,
        //    .dstBinding = 1,
        //    .descriptorCount = 1,
        //    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        //    .pImageInfo = &mi.mat_dscs.met_rough_dscs.metal_rough_desc,
        //}

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(write_desc_sets.size()), write_desc_sets.data(), 0, nullptr);

        for (auto& pd : mi.pds)
        {
            pd.xform_desc.buffer = d.uni_buff_mem.buffer;
            pd.xform_desc.offset = uni_buff_offset;
            pd.xform_desc.range = sizeof(mat4);

            std::vector<VkDescriptorPoolSize> pool_sizes(1);
            pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            pool_sizes[0].descriptorCount = 1;

            // create xform desc pools/sets per primitive
            pd.desc_pool = vk_descriptor_pool::create(device, 1, pool_sizes, "per primtive xform desc pool");
            std::vector<VkDescriptorSetLayout> dsls(1, d.vtx_pipeline_d_sets.dsls[2]);
            pd.desc_set = vk_descriptor_sets::allocate(device, pd.desc_pool, dsls, "per primitive xform desc set ")[0];

            std::vector<VkWriteDescriptorSet >write_desc_sets(1);
            write_desc_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_desc_sets[0].dstSet = pd.desc_set;
            write_desc_sets[0].descriptorCount = 1;
            write_desc_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write_desc_sets[0].pBufferInfo = &pd.xform_desc;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(write_desc_sets.size()), write_desc_sets.data(), 0, nullptr);

            uni_buff_offset += aligned_mat_size;
        }
    }

    return d;
}

void scene_data::destroy(const data d, const VkDevice device)
{
    for (auto const& mi : d.mis)
    {
        vk_image::destroy(mi.mat_dscs.met_rough_dscs.base_color_image, device);
        vk_image_view::destroy(mi.mat_dscs.met_rough_dscs.base_color_desc.imageView, device);
    }
    vk_device_memory::free(d.images_memory, device);
    host_buffer_memory::destroy(d.uni_buff_mem, device);
    host_buffer_memory::destroy(d.desc_buff_mem, device);
    device_buffer_memory::destroy(d.geom_buff_mem, device);
    vk_descriptor_pool::destroy(d.desc_pool, device);
    vk_graphics_pipeline::destroy(d.mesh_pipeline_d_sets, device);
    vk_graphics_pipeline::destroy(d.mesh_pipeline_d_buff, device);
    vk_graphics_pipeline::destroy(d.vtx_pipeline_d_sets, device);
    vk_graphics_pipeline::destroy(d.vtx_pipeline_d_buff, device);
}
