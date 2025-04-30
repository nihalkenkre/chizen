#include "vk_renderer.hpp"

#include <iostream>
#include <meshoptimizer/src/meshoptimizer.h>
#include <cglm/include/cglm/cglm.h>
#include <filesystem>

#include <Shlwapi.h>

#include <SPIRV-Reflect/spirv_reflect.h>

constexpr uint8_t MAX_VERTICES = 64;
constexpr uint8_t MAX_TRIANGLES = 124;

inline static VkDeviceSize ALIGNED_SIZE(VkDeviceSize value, VkDeviceSize alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

inline static VkViewport RECT_TO_VIEWPORT(const RECT& rect)
{
    const VkViewport v = {
        .x = 0,
        .y = 0,
        .width = static_cast<float>(rect.right - rect.left),
        .height = static_cast<float>(rect.bottom - rect.top),
        .minDepth = 0,
        .maxDepth = 1,
    };

    return v;
}

inline static RECT VIEWPORT_TO_RECT(const VkViewport& viewport)
{
    const RECT r = {
        .left = static_cast<LONG>(viewport.x),
        .top = static_cast<LONG>(viewport.y),
        .right = static_cast<LONG>(viewport.width),
        .bottom = static_cast<LONG>(viewport.height),
    };

    return r;
}

inline static RECT SANITIZE_RECT_FOR_RENDER(const RECT& rect)
{
    const RECT r = {
        .left = 0,
        .top = 0,
        .right = rect.right - rect.left,
        .bottom = rect.bottom - rect.top,
    };

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
    acq_sig_sem = vk_semaphore::create(device, false, "acquire signal semaphore");
    acq_wait_sem = vk_semaphore::create(device, true, "acquire wait semaphore");

    // signalling so that the render function does not stall on vkAcquireNextImage the first time
    const VkSemaphoreSignalInfo sem_sig_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
        .semaphore = acq_wait_sem.semaphore,
        .value = ++acq_wait_sem_val,
    };
    vkSignalSemaphore(device, &sem_sig_info);

    GetWindowRect(h_wnd, &wnd_rect);
    wnd_rect = SANITIZE_RECT_FOR_RENDER(wnd_rect);
    viewport = RECT_TO_VIEWPORT(wnd_rect);

    VkDeviceQueueInfo2 queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
        .queueFamilyIndex = phy_dev.q_fly_idx,
        .queueIndex = 0,
    };
    vkGetDeviceQueue2(device, &queue_info, &gfx_q);

    queue_info.queueIndex = 1;
    vkGetDeviceQueue2(device, &queue_info, &xfer_q);

    xfer_cmd_pool = vk_command_pool::create(device, phy_dev.q_fly_idx, 1, "xfer command pool");
}

void vk_renderer::import_scene_data(const std::string& file_path)
{
    sd = scene_data::create(file_path, phy_dev, surface, device, xfer_q, xfer_cmd_pool.cmd_buffs[0]);
}

void vk_renderer::resize(const UINT width, const UINT height)
{
    VK_CHECK("wait for present fence", vkWaitForFences(device, 1, &swapchain.present_fences[img_idx], VK_TRUE, UINT64_MAX));
    VK_CHECK("reset present fence", vkResetFences(device, 1, &swapchain.present_fences[img_idx]));

    VK_CHECK("get surface capabilities", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phy_dev.phy_dev, surface.surface, &surface.surf_caps));

    vk_swapchain::destroy(swapchain, device);
    swapchain = vk_swapchain::create(device, surface, phy_dev, "swapchain");
}

void vk_renderer::begin_frame()
{
    uint64_t wait_values = acq_wait_sem_val;

    const VkSemaphoreWaitInfo wait_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores = &acq_wait_sem.semaphore,
        .pValues = &wait_values,
    };
    vkWaitSemaphores(device, &wait_info, UINT64_MAX);

    VK_CHECK("acquire image index", vkAcquireNextImageKHR(device, swapchain.swapchain, UINT64_MAX, acq_sig_sem.semaphore, VK_NULL_HANDLE, &img_idx));
    VK_CHECK("reset command buffer", vkResetCommandBuffer(swapchain.cmd_buffs[img_idx], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

    const VkImageMemoryBarrier2 img_mem_barr2 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = 0,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain.images[img_idx],
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        },
    };

    const VkDependencyInfo dep_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &img_mem_barr2,
    };

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(swapchain.cmd_buffs[img_idx], &begin_info);
    vkCmdPipelineBarrier2(swapchain.cmd_buffs[img_idx], &dep_info);
}

void vk_renderer::clear_frame(const float color[])
{
    const VkRenderingAttachmentInfoKHR color_attachment_infos[] = {
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchain.image_views[img_idx],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {
            .color = {
                .float32 = {
                    color[0], color[1], color[2], color[3]
                },
            },
        },
    },
    };

    const VkRenderingInfoKHR rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {
            .extent = surface.surf_caps.currentExtent,
        },
        .layerCount = 1,
        .colorAttachmentCount = _countof(color_attachment_infos),
        .pColorAttachments = color_attachment_infos,
    };

    vkCmdBeginRendering(swapchain.cmd_buffs[img_idx], &rendering_info);
}

void vk_renderer::render_world()
{
    const VkViewport viewports[] = {
        {
            .width = static_cast<float>(surface.surf_caps.currentExtent.width),
            .height = static_cast<float>(surface.surf_caps.currentExtent.height),
            .minDepth = 0,
            .maxDepth = 1,
        },
    };

    const VkRect2D scissors[] = {
        {
            .extent = surface.surf_caps.currentExtent,
        },
    };

    vkCmdSetScissor(swapchain.cmd_buffs[img_idx], 0, _countof(scissors), scissors);
    vkCmdSetPrimitiveTopology(swapchain.cmd_buffs[img_idx], VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    vkCmdSetPolygonModeEXT(swapchain.cmd_buffs[img_idx], VK_POLYGON_MODE_FILL);
    vkCmdSetViewport(swapchain.cmd_buffs[img_idx], 0, _countof(viewports), viewports);

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

    // set 0 scene level
    vkCmdBindDescriptorSets(swapchain.cmd_buffs[img_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, sd.vtx_pipeline_d_sets.pipeline_layout, 0, 1, &sd.desc_sets[0], 0, nullptr);

    for (auto const& mi : sd.mis)
    {
        for (auto const& pd : mi.pds)
        {
            const VkBuffer buffers[] = {
                sd.geom_buff_mem.buffer,
                sd.geom_buff_mem.buffer,
            };

            const VkDeviceSize offsets[] = {
                pd.vsi.positions_offset,
                pd.vsi.uvs_offsets,
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

    const VkImageMemoryBarrier2 img_mem_barr = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain.images[img_idx],
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

    vkCmdPipelineBarrier2(swapchain.cmd_buffs[img_idx], &dep_info);

    vkEndCommandBuffer(swapchain.cmd_buffs[img_idx]);

    const VkSemaphoreSubmitInfo wait_sem_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = acq_sig_sem.semaphore,
            .stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        }
    };

    const VkSemaphoreSubmitInfo sig_sem_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = swapchain.rndr_semaphores[img_idx],
            .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = acq_wait_sem.semaphore,
            .value = ++acq_wait_sem_val,
            .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        }
    };

    const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = swapchain.cmd_buffs[img_idx],
        }
    };

    const VkSubmitInfo2 submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = _countof(wait_sem_infos),
        .pWaitSemaphoreInfos = wait_sem_infos,
        .commandBufferInfoCount = _countof(cmd_buff_infos),
        .pCommandBufferInfos = cmd_buff_infos,
        .signalSemaphoreInfoCount = _countof(sig_sem_infos),
        .pSignalSemaphoreInfos = sig_sem_infos,
    };

    vkQueueSubmit2(gfx_q, 1, &submit_info, VK_NULL_HANDLE);

    const VkSwapchainPresentFenceInfoEXT present_fence_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT,
        .swapchainCount = 1,
        .pFences = &swapchain.present_fences[img_idx],
    };

    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = &present_fence_info,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &swapchain.rndr_semaphores[img_idx],
        .swapchainCount = 1,
        .pSwapchains = &swapchain.swapchain,
        .pImageIndices = &img_idx,
    };

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

scene_data::data scene_data::create(const std::string& file_path, const vk_phy_dev::data& phy_dev, const vk_surface::data& surface, const VkDevice& device, const VkQueue& xfer_q, const VkCommandBuffer& cmd_buff)
{
    data d;

    char curr_dir[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA(NULL), curr_dir, MAX_PATH);

    PathRemoveFileSpecA(curr_dir);

    std::string tmp(curr_dir);
    tmp.append("\\shaders\\");

    for (auto const& file : std::filesystem::directory_iterator(std::filesystem::path(tmp)))
    {
        if (std::filesystem::is_directory(file))
        {
            for (auto const& pipeline_type : std::filesystem::directory_iterator(std::filesystem::path(file)))
            {
                if (std::filesystem::is_directory(pipeline_type)) {

                    if (pipeline_type.path().string().find("mesh") != std::string::npos) {
                        d.mesh_pipeline_d_sets = vk_graphics_pipeline::create(device, pipeline_type.path().string(), CHI_PIPELINE_TYPE::MESH, surface.format.format, phy_dev.desc_buff_props, "mesh pipeline d sets");
#ifdef DESC_BUFFER
                        d.mesh_d_buff_pipeline = vk_graphics_pipeline::create(device, pipeline_type.path().string(), CHI_PIPELINE_TYPE::MESH, surface.format.format, phy_dev.desc_buff_props);
#endif // DESC_BUFFER
                    }
                    else if (pipeline_type.path().string().find("vertex") != std::string::npos) {
                        d.vtx_pipeline_d_sets = vk_graphics_pipeline::create(device, pipeline_type.path().string(), CHI_PIPELINE_TYPE::VERTEX, surface.format.format, phy_dev.desc_buff_props, "vtx pipeline d sets");
#ifdef DESC_BUFFER
                        d.vtx_pipeline_d_buff = vk_graphics_pipeline::create(device, pipeline_type.path().string(), CHI_PIPELINE_TYPE::VERTEX, surface.format.format, phy_dev.desc_buff_props);
#endif // DESC_BUFFER
                    }
                }
            }
        }
    }

    d.cam_xform_d_buff_desc.resize(phy_dev.desc_buff_props.uniformBufferDescriptorSize);

    cgltf_options options = {};
    cgltf_data* data = nullptr;

    // Meshlets data (positions, meshlets, vertices, indices)
    std::vector<uint8_t> mesh_geom_data;
    // Vertex, index data
    std::vector<uint8_t> vertex_geom_data;
    std::vector<uint8_t> xform_data;

    mat4 identity_mat;
    glm_mat4_identity(identity_mat);

    mat4 v;
    vec3 eye = { 0, 0, 2 };
    vec3 center = { 0, 0, 0 };
    vec3 up = { 0, 1, 0 };
    glm_lookat(eye, center, up, v);

    mat4 p;
    glm_perspective(90.f, (float)surface.surf_caps.currentExtent.width / (float)surface.surf_caps.currentExtent.height, 0.1, 1000.f, p);
    p[1][1] *= -1;

    mat4 vp;
    glm_mul(p, v, vp);

    VkDeviceSize aligned_mat_size = ALIGNED_SIZE(sizeof(vp), phy_dev.props.properties.limits.minUniformBufferOffsetAlignment);

    xform_data.resize(aligned_mat_size);
    std::memcpy(xform_data.data(), vp, sizeof(vp));

    if (cgltf_parse_file(&options, file_path.c_str(), &data) != cgltf_result_success ||
        cgltf_validate(data) != cgltf_result_success ||
        cgltf_load_buffers(&options, data, file_path.c_str()) != cgltf_result_success)
    {
        std::cerr << "ERR Could not parse gltf file\n";
    }

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
            std::vector<uint8_t> positions;
            std::vector<uint8_t> uvs;

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
                    size_t curr_geom_size = positions.size();
                    positions.resize(positions.size() + (curr_attr->data->count * curr_attr->data->stride));
                    std::memcpy(&positions[curr_geom_size], (void*)((ULONG_PTR)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->count * curr_attr->data->stride);
                }
                else if (std::strcmp(curr_attr->name, "TEXCOORD_0") == 0)
                {
                    size_t curr_geom_size = uvs.size();
                    uvs.resize(uvs.size() + (curr_attr->data->count * curr_attr->data->stride));
                    std::memcpy(&uvs[curr_geom_size], (void*)((ULONG_PTR)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->count * curr_attr->data->stride);
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

                vertex_geom_data.resize(vertex_geom_data.size() + positions.size());
                std::memcpy(vertex_geom_data.data() + vertex_geom_data_size, positions.data(), positions.size());

                // copy uvs data
                vertex_geom_data_size = vertex_geom_data.size();
                pd.vsi.uvs_offsets = vertex_geom_data_size;

                vertex_geom_data.resize(vertex_geom_data.size() + uvs.size());
                std::memcpy(vertex_geom_data.data() + vertex_geom_data_size, uvs.data(), uvs.size());
            }

            cgltf_material* mat = curr_prim->material;

            if (mat->has_pbr_metallic_roughness)
            {
                metal_rough_descriptors mr_dscs = {
                    .base_color_factors = {
                        mat->pbr_metallic_roughness.base_color_factor[0],
                        mat->pbr_metallic_roughness.base_color_factor[1],
                        mat->pbr_metallic_roughness.base_color_factor[2],
                        mat->pbr_metallic_roughness.base_color_factor[3],
                    },
                    .metalness_factor = mat->pbr_metallic_roughness.metallic_factor,
                    .roughness_factor = mat->pbr_metallic_roughness.roughness_factor,
                };
            }
            else
            {
                pd.mat_dscs.met_rough_dscs = {
                    .base_color_desc = VK_NULL_HANDLE,
                    .metal_rough_desc = VK_NULL_HANDLE,
                };
            }

            uint64_t mat_id = std::hash<std::string>{}(curr_prim->material->name);
            auto it = std::find_if(d.mis.begin(), d.mis.end(), [mat_id](const material_info& mi) { return mi.id == mat_id; });

            if (it == d.mis.end())
            {
                material_info mi = {
                    .id = mat_id,
                    .pds = {pd},
                };

                d.mis.push_back(mi);
            }
            else
            {
                it->pds.push_back(std::move(pd));
            }
        }
    }

    cgltf_free(data);

    d.geom_buff_mem = device_buffer_memory::create(
        device, phy_dev.mem_props, 0,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        vertex_geom_data, xfer_q, cmd_buff, "geom buffer memory");
    d.uni_buff_mem = host_buffer_memory::create(
        device, phy_dev.mem_props, 0,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        xform_data, "uniform buffer memory");

    {
        // create descriptors for vertex pipeline

        VkDeviceSize uni_buff_offset = 0;

        d.cam_xform_d_set_desc = {
            .buffer = d.uni_buff_mem.buffer,
            .offset = uni_buff_offset,
            .range = sizeof(mat4),
        };

        uni_buff_offset += ALIGNED_SIZE(sizeof(mat4), phy_dev.props.properties.limits.minUniformBufferOffsetAlignment);

        for (auto& mi : d.mis)
        {
            for (auto& pd : mi.pds)
            {
                pd.vsi.xform_d_set_desc = {
                    .buffer = d.uni_buff_mem.buffer,
                    .offset = uni_buff_offset,
                    .range = sizeof(mat4),
                };

                uni_buff_offset += ALIGNED_SIZE(sizeof(mat4), phy_dev.props.properties.limits.minUniformBufferOffsetAlignment);
            }
        }
    }

    std::vector<VkDescriptorPoolSize> pool_sizes = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
        },
    };

    d.desc_pool = vk_descriptor_pool::create(device, 1, pool_sizes);
    std::vector<VkDescriptorSetLayout> dsls(1, d.vtx_pipeline_d_sets.dsls[0]);
    d.desc_sets = vk_descriptor_sets::allocate(device, d.desc_pool, dsls);

    const VkWriteDescriptorSet write_desc_sets[] = {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = d.desc_sets[0],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &d.cam_xform_d_set_desc,
        },
    };

    vkUpdateDescriptorSets(device, _countof(write_desc_sets), write_desc_sets, 0, nullptr);

    return d;
}

void scene_data::destroy(const data d, const VkDevice device)
{
    host_buffer_memory::destroy(d.uni_buff_mem, device);
    host_buffer_memory::destroy(d.desc_buff_mem, device);
    device_buffer_memory::destroy(d.geom_buff_mem, device);
    vk_descriptor_pool::destroy(d.desc_pool, device);
    vk_graphics_pipeline::destroy(d.mesh_pipeline_d_sets, device);
    vk_graphics_pipeline::destroy(d.mesh_pipeline_d_buff, device);
    vk_graphics_pipeline::destroy(d.vtx_pipeline_d_sets, device);
    vk_graphics_pipeline::destroy(d.vtx_pipeline_d_buff, device);
}
