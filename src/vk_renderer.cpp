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
    VkResult result = volkInitialize();
    instance = std::make_unique<vk_instance>();
    volkLoadInstance(instance->instance);
    surface = std::make_unique<vk_surface>(instance->instance, GetModuleHandleA(nullptr), h_wnd);
    phy_dev = std::make_unique<vk_phydev>(instance->instance, surface.get());
    device = std::make_unique<vk_device>(phy_dev->phy_dev, phy_dev->q_fly_idx, phy_dev->q_count);
    swapchain = std::make_unique<vk_swapchain>(device->device, surface.get(), phy_dev.get());
    acq_sig_sem = std::make_unique<vk_semaphore>(device->device, false);
    acq_wait_sem = std::make_unique<vk_semaphore>(device->device, true);

    const VkSemaphoreSignalInfo sem_sig_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
        .semaphore = acq_wait_sem->semaphore,
        .value = ++acq_wait_sem_val,
    };
    vkSignalSemaphore(device->device, &sem_sig_info);

    GetWindowRect(h_wnd, &wnd_rect);
    wnd_rect = SANITIZE_RECT_FOR_RENDER(wnd_rect);
    viewport = RECT_TO_VIEWPORT(wnd_rect);

    VkDeviceQueueInfo2 queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
        .queueFamilyIndex = phy_dev->q_fly_idx,
        .queueIndex = 0,
    };
    vkGetDeviceQueue2(device->device, &queue_info, &gfx_q);

    queue_info.queueIndex = 1;
    vkGetDeviceQueue2(device->device, &queue_info, &xfer_q);

    xfer_cmd_pool = std::make_unique<vk_command_pool>(device->device, phy_dev->q_fly_idx, 1);

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

                    if (pipeline_type.path().string().find("mesh") != std::string::npos)
                        mesh_d_sets_pipeline = std::make_unique<vk_graphics_pipeline>(device->device, pipeline_type.path().string(), CHI_PIPELINE_TYPE::MESH, surface->format.format, phy_dev->desc_buff_props);
#ifdef DESC_BUFFER
                    mesh_d_buff_pipeline = std::make_unique<vk_graphics_pipeline>(device->device, pipeline_type.path().string(), CHI_PIPELINE_TYPE::MESH, surface->format.format, phy_dev->desc_buff_props);
#endif // DESC_BUFFER
                    else if (pipeline_type.path().string().find("vertex") != std::string::npos) {
                        vtx_pipeline_d_sets = std::make_unique<vk_graphics_pipeline>(device->device, pipeline_type.path().string(), CHI_PIPELINE_TYPE::VERTEX, surface->format.format, phy_dev->desc_buff_props);
#ifdef DESC_BUFFER
                        vtx_pipeline_d_buff = std::make_unique<vk_graphics_pipeline>(device->device, pipeline_type.path().string(), CHI_PIPELINE_TYPE::VERTEX, surface->format.format, phy_dev->desc_buff_props);
#endif // DESC_BUFFER
                    }
                }
            }
        }
    }
}

void vk_renderer::import_scene_data(const std::string& file_path)
{
    sd = std::make_unique<scene_data>();
    sd->cam_xform_d_buff_desc.resize(phy_dev->desc_buff_props.uniformBufferDescriptorSize);

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
    glm_perspective(90.f, (float)surface->surf_caps.currentExtent.width / (float)surface->surf_caps.currentExtent.height, 0.1, 1000.f, p);
    p[1][1] *= -1;

    mat4 vp;
    glm_mul(p, v, vp);

    VkDeviceSize aligned_mat_size = ALIGNED_SIZE(sizeof(vp), phy_dev->props.properties.limits.minUniformBufferOffsetAlignment);

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

            std::vector<uint32_t>indices;
            std::vector<float3> positions;

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

            for (cgltf_size a = 0; a < curr_prim->attributes_count; ++a)
            {
                cgltf_attribute* curr_attr = curr_prim->attributes + a;

                if (std::strcmp(curr_attr->name, "POSITION") == 0)
                {
                    size_t curr_geom_size = positions.size();
                    positions.resize(positions.size() + curr_attr->data->count);

                    std::memcpy(&positions[curr_geom_size], (void*)((ULONG_PTR)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->count * sizeof(float3));
                }
            }

            primitive_data pd = {
                .vsi = {
                    .indices_count = static_cast<uint32_t>(indices.size()),
                },
            };
            pd.vsi.xform_d_buff_desc.resize(phy_dev->desc_buff_props.uniformBufferDescriptorSize);

            // details for mesh pipeline
            {
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
            }

            // details for vertex pipeline
            {
                size_t geom_data_size = vertex_geom_data.size();
                pd.vsi.indices_offset = geom_data_size;

                vertex_geom_data.resize(vertex_geom_data.size() + indices.size() * sizeof(indices[0]));

                std::memcpy(vertex_geom_data.data() + geom_data_size, indices.data(), indices.size() * sizeof(indices[0]));

                geom_data_size = vertex_geom_data.size();
                pd.vsi.positions_offset = geom_data_size;

                vertex_geom_data.resize(vertex_geom_data.size() + positions.size() * sizeof(positions[0]));
                std::memcpy(vertex_geom_data.data() + geom_data_size, positions.data(), positions.size() * sizeof(positions[0]));
            }

            uint64_t mat_id = std::hash<std::string>{}(curr_prim->material->name);
            auto it = std::find_if(sd->mis.begin(), sd->mis.end(), [mat_id](const material_info& mi) { return mi.id == mat_id; });

            if (it == sd->mis.end())
            {
                material_info mi = {
                    .id = mat_id,
                    .pds = {pd},
                };

                sd->mis.push_back(mi);
            }
            else
            {
                it->pds.push_back(pd);
            }
        }
    }

    cgltf_free(data);

    std::vector<uint8_t> xform_data_t(1024, 0xFF);
    sd->geom_buff_mem = std::make_unique<device_buffer_memory>(
        device->device, phy_dev->mem_props, 0,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        vertex_geom_data, xfer_q, xfer_cmd_pool->cmd_buffs[0]);
    sd->uni_buff_mem = std::make_unique<host_buffer_memory>(
        device->device, phy_dev->mem_props, 0,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        xform_data);

    uint32_t max_desc_sets = 0;
    {
        // create descriptors for vertex pipeline

        VkDeviceSize uni_buff_offset = 0;
        VkDeviceSize desc_data_size = 0;

        VkDescriptorAddressInfoEXT addr_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
            .address = sd->uni_buff_mem->addr + uni_buff_offset,
            .range = sizeof(mat4),
        };

        VkDescriptorDataEXT desc_data = {
            .pUniformBuffer = &addr_info,
        };

        VkDescriptorGetInfoEXT desc_get_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .data = desc_data,
        };

        sd->cam_xform_d_set_desc = {
            .buffer = sd->uni_buff_mem->buffer->buffer,
            .offset = uni_buff_offset,
            .range = sizeof(mat4),
        };
#ifdef DESC_BUFFER
        vkGetDescriptorEXT(
            device->device, &desc_get_info, phy_dev->desc_buff_props.uniformBufferDescriptorSize,
            reinterpret_cast<void*>(sd->cam_xform_d_buff_desc.data())
        );
#endif // DESC_BUFFER

        uni_buff_offset += ALIGNED_SIZE(sizeof(mat4), phy_dev->props.properties.limits.minUniformBufferOffsetAlignment);

        for (auto& mi : sd->mis)
        {
            for (auto& pd : mi.pds)
            {
                addr_info = {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
                    .address = sd->uni_buff_mem->addr + uni_buff_offset,
                    .range = sizeof(mat4),
                };

                desc_data = {
                    .pUniformBuffer = &addr_info,
                };

                desc_get_info = {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .data = desc_data,
                };
#ifdef DESC_BUFFER
                vkGetDescriptorEXT(device->device, &desc_get_info,
                    phy_dev->desc_buff_props.uniformBufferDescriptorSize,
                    reinterpret_cast<void*>(pd.vsi.xform_d_buff_desc.data()));
#endif // DESC_BUFFER
                pd.vsi.xform_d_set_desc = {
                    .buffer = sd->uni_buff_mem->buffer->buffer,
                    .offset = uni_buff_offset,
                    .range = sizeof(mat4),
                };

                uni_buff_offset += ALIGNED_SIZE(sizeof(mat4), phy_dev->props.properties.limits.minUniformBufferOffsetAlignment);
#ifdef DESC_BUFFER
                desc_data_size += vtx_pipeline_d_buff->dsl_infos[0].aligned_size;
#endif // DESC_BUFFER

                ++max_desc_sets;
            }
        }

#ifdef DESC_BUFFER
        sd->desc_buff_mem = std::make_unique<host_buffer_memory>(device->device, phy_dev->mem_props, desc_data_size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        );
#endif // DESC_BUFFER
    }

    VkDeviceSize desc_buff_offset = 0;

    size_t curr_mat_idx = 0;

#ifdef DESC_BUFFER
    for (auto const& mi : sd->mis)
    {
        for (auto const& pd : mi.pds)
        {
            // copy cam xform to the desc buffer
            std::memcpy((void*)((uint64_t)sd->desc_buff_mem->map + desc_buff_offset + vtx_pipeline_d_buff->dsl_infos[0].binding_infos[0].offset), sd->cam_xform_d_buff_desc.data(), sd->cam_xform_d_buff_desc.size());
            // copy pd xform to desc buff
            std::memcpy((void*)((uint64_t)sd->desc_buff_mem->map + desc_buff_offset + vtx_pipeline_d_buff->dsl_infos[0].binding_infos[1].offset), pd.vsi.xform_d_buff_desc.data(), pd.vsi.xform_d_buff_desc.size());

            desc_buff_offset += vtx_pipeline_d_buff->dsl_infos[0].aligned_size;
        }
    }
#endif // DESC_BUFFER

    std::vector<VkDescriptorPoolSize> pool_sizes = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = max_desc_sets * 2,
        },
    };

    sd->desc_pool = std::make_unique<vk_descriptor_pool>(device->device, max_desc_sets, pool_sizes);
    std::vector<VkDescriptorSetLayout> dsls(max_desc_sets, vtx_pipeline_d_sets->dsls[0]);
    sd->desc_sets = std::make_unique<vk_descriptor_sets>(device->device, sd->desc_pool->descriptor_pool, dsls, max_desc_sets);

    size_t desc_set_idx = 0;
    for (auto& mi : sd->mis)
    {
        for (auto& pd : mi.pds)
        {
            const VkWriteDescriptorSet write_desc_sets[] = {
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = sd->desc_sets->desc_sets[desc_set_idx],
                    .dstBinding = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &sd->cam_xform_d_set_desc,
                },
                {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = sd->desc_sets->desc_sets[desc_set_idx],
                    .dstBinding = 1,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &pd.vsi.xform_d_set_desc,
                },
            };

            vkUpdateDescriptorSets(device->device, _countof(write_desc_sets), write_desc_sets, 0, nullptr);

            ++desc_set_idx;
        }
    }

    /*
    {
        // camera VP dsl * 2 for swapchain count, because data will change every frame
        // taking dsl size since we will need the create descriptors for the entire desc set
        VkDeviceSize desc_buff_size = ALIGNED_SIZE(pbr_pipeline->dsl_infos[0].size, phy_dev->desc_buff_props.descriptorBufferOffsetAlignment);
        desc_buff_size += ALIGNED_SIZE(pbr_pipeline->dsl_infos[0].size, phy_dev->desc_buff_props.descriptorBufferOffsetAlignment);

        // per prim dsl only one before data wont change
        for (auto const& mi : sd->mis)
        {
            for (auto const& pd : mi.pds)
                desc_buff_size += ALIGNED_SIZE(pbr_pipeline->dsl_infos[1].size, phy_dev->desc_buff_props.descriptorBufferOffsetAlignment);
        }

        // Holds the descriptor data
        std::vector<uint8_t> desc_buff_data(desc_buff_size);

        // Offset into the desc_buff_data
        VkDeviceSize desc_buff_offset = 0;

        // Offset into the uniform buffer
        VkDeviceSize uni_buff_offset = 0;

        for (uint8_t i = 0; i < swapchain->images_count; ++i)
        {
            VkDescriptorAddressInfoEXT desc_addr_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
                .address = sd->uni_buff_mem->addr + uni_buff_offset,
                .range = sizeof(mat4),
            };

            VkDescriptorDataEXT desc_data = {
                .pUniformBuffer = &desc_addr_info,
            };

            VkDescriptorGetInfoEXT desc_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .data = desc_data,
            };

            vkGetDescriptorEXT(device->device, &desc_info, phy_dev->desc_buff_props.uniformBufferDescriptorSize, reinterpret_cast<void*>((uint64_t)desc_buff_data.data() + desc_buff_offset));

            uni_buff_offset += ALIGNED_SIZE(sizeof(mat4), phy_dev->props.properties.limits.minUniformBufferOffsetAlignment);
            desc_buff_offset += ALIGNED_SIZE(pbr_pipeline->dsl_infos[0].size, phy_dev->desc_buff_props.descriptorBufferOffsetAlignment);
        }

        for (auto& mi : sd->mis)
        {
            for (auto& pd : mi.pds)
            {
                // Holds the offset into desc_buff_data for bindings
                VkDeviceSize desc_buff_binding_offset = desc_buff_offset + pbr_pipeline->dsl_infos[1].binding_infos[0].offset;

                // xform uniform buffer binding 0
                {
                    VkDescriptorAddressInfoEXT desc_addr_info = {
                        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
                        .address = sd->uni_buff_mem->addr + uni_buff_offset,
                        .range = sizeof(mat4),
                    };

                    VkDescriptorDataEXT desc_data = {
                        .pUniformBuffer = &desc_addr_info,
                    };

                    VkDescriptorGetInfoEXT desc_info = {
                        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .data = desc_data,
                    };

                    vkGetDescriptorEXT(device->device, &desc_info, phy_dev->desc_buff_props.uniformBufferDescriptorSize, reinterpret_cast<void*>((uint64_t)desc_buff_data.data() + desc_buff_binding_offset));

                    uni_buff_offset += ALIGNED_SIZE(sizeof(mat4), phy_dev->props.properties.limits.minUniformBufferOffsetAlignment);
                }

                // meshlet storage buffer bindings 1 - 4
                {
                    for (uint8_t d = 1; d < 5; ++d)
                    {
                        desc_buff_binding_offset = desc_buff_offset + pbr_pipeline->dsl_infos[1].binding_infos[d].offset;

                        VkDescriptorAddressInfoEXT desc_addr_info = {
                            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
                            .address = sd->geom_buff_mem->addr + pd.geom_data_offsets[d % 1],
                            .range = pd.geom_data_sizes[d % 1],
                        };

                        VkDescriptorDataEXT desc_data = {
                            .pStorageBuffer = &desc_addr_info,
                        };

                        VkDescriptorGetInfoEXT desc_info = {
                            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                            .data = desc_data,
                        };

                        vkGetDescriptorEXT(device->device, &desc_info, phy_dev->desc_buff_props.storageBufferDescriptorSize, reinterpret_cast<void*>((uint64_t)desc_buff_data.data() + desc_buff_binding_offset));
                    }
                }

                pd.geom_descs_offset = desc_buff_offset;
                desc_buff_offset += ALIGNED_SIZE(pbr_pipeline->dsl_infos[1].size, phy_dev->desc_buff_props.descriptorBufferOffsetAlignment);
            }
        }

        sd->desc_buff_mem = std::make_unique<device_buffer_memory>(
            device->device, phy_dev->mem_props,
            0,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            desc_buff_data,
            xfer_q,
            xfer_cmd_pool->cmd_buffs[0]
        );
    }
    */
}

void vk_renderer::resize(const UINT width, const UINT height)
{
    VK_CHECK("wait for present fence", vkWaitForFences(device->device, 1, &swapchain->present_fences[img_idx], VK_TRUE, UINT64_MAX));
    VK_CHECK("reset present fence", vkResetFences(device->device, 1, &swapchain->present_fences[img_idx]));

    VK_CHECK("get surface capabilities", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phy_dev->phy_dev, surface->surface, &surface->surf_caps));

    swapchain.reset();
    swapchain = std::make_unique<vk_swapchain>(device->device, surface.get(), phy_dev.get());
}

void vk_renderer::begin_frame()
{
    uint64_t wait_values = acq_wait_sem_val;

    const VkSemaphoreWaitInfo wait_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores = &acq_wait_sem->semaphore,
        .pValues = &wait_values,
    };
    vkWaitSemaphores(device->device, &wait_info, UINT64_MAX);

    VK_CHECK("acquire image index", vkAcquireNextImageKHR(device->device, swapchain->swapchain, UINT64_MAX, acq_sig_sem->semaphore, VK_NULL_HANDLE, &img_idx));
    VK_CHECK("reset command buffer", vkResetCommandBuffer(swapchain->cmd_buffs[img_idx], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

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
        .image = swapchain->images[img_idx],
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

    vkBeginCommandBuffer(swapchain->cmd_buffs[img_idx], &begin_info);
    vkCmdPipelineBarrier2(swapchain->cmd_buffs[img_idx], &dep_info);
}

void vk_renderer::clear_frame(const float color[])
{
    const VkRenderingAttachmentInfoKHR color_attachment_infos[] = {
    {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchain->image_views[img_idx],
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
            .extent = surface->surf_caps.currentExtent,
        },
        .layerCount = 1,
        .colorAttachmentCount = _countof(color_attachment_infos),
        .pColorAttachments = color_attachment_infos,
    };

    vkCmdBeginRendering(swapchain->cmd_buffs[img_idx], &rendering_info);
}

void vk_renderer::render_world()
{
    const VkViewport viewports[] = {
        {
            .width = static_cast<float>(surface->surf_caps.currentExtent.width),
            .height = static_cast<float>(surface->surf_caps.currentExtent.height),
            .minDepth = 0,
            .maxDepth = 1,
        },
    };

    const VkRect2D scissors[] = {
        {
            .extent = surface->surf_caps.currentExtent,
        },
    };

    vkCmdSetScissor(swapchain->cmd_buffs[img_idx], 0, _countof(scissors), scissors);
    vkCmdSetPrimitiveTopology(swapchain->cmd_buffs[img_idx], VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    vkCmdSetPolygonModeEXT(swapchain->cmd_buffs[img_idx], VK_POLYGON_MODE_FILL);
    vkCmdSetViewport(swapchain->cmd_buffs[img_idx], 0, _countof(viewports), viewports);

    size_t desc_set_idx = 0;
    vkCmdBindPipeline(swapchain->cmd_buffs[img_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, vtx_pipeline_d_sets->pipeline);

    for (auto const& mi : sd->mis)
    {
        for (auto const& pd : mi.pds)
        {
            vkCmdBindDescriptorSets(swapchain->cmd_buffs[img_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, vtx_pipeline_d_sets->pipeline_layout, 0, 1, &sd->desc_sets->desc_sets[desc_set_idx], 0, nullptr);

            const VkBuffer buffers[] = {
                sd->geom_buff_mem->buffer->buffer,
            };

            const VkDeviceSize offsets[] = {
                pd.vsi.positions_offset,
            };

            vkCmdBindVertexBuffers(swapchain->cmd_buffs[img_idx], 0, _countof(buffers), buffers, offsets);
            vkCmdBindIndexBuffer(swapchain->cmd_buffs[img_idx], sd->geom_buff_mem->buffer->buffer, pd.vsi.indices_offset, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(swapchain->cmd_buffs[img_idx], pd.vsi.indices_count, 1, 0, 0, 0);

            ++desc_set_idx;
        }
    }

#ifdef DESC_BUFFER
    vkCmdBindPipeline(swapchain->cmd_buffs[img_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, vtx_pipeline_d_buff->pipeline);
    const VkDescriptorBufferBindingInfoEXT binding_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
            .address = sd->desc_buff_mem->addr,
            .usage = sd->desc_buff_mem->usage,
        },
    };

    vkCmdBindDescriptorBuffersEXT(swapchain->cmd_buffs[img_idx], _countof(binding_infos), binding_infos);

    const uint32_t buff_idxs[] = { 0 };
    VkDeviceSize desc_buff_offset = 0;

    for (auto const& mi : sd->mis)
    {
        for (auto const& pd : mi.pds)
        {
            vkCmdSetDescriptorBufferOffsetsEXT(swapchain->cmd_buffs[img_idx], VK_PIPELINE_BIND_POINT_GRAPHICS, vtx_pipeline_d_buff->pipeline_layout, 0, _countof(buff_idxs), buff_idxs, &desc_buff_offset);

            const VkBuffer buffers[] = {
                sd->geom_buff_mem->buffer->buffer,
            };

            const VkDeviceSize offsets[] = {
                pd.vsi.positions_offset,
            };

            vkCmdBindVertexBuffers(swapchain->cmd_buffs[img_idx], 0, _countof(buffers), buffers, offsets);
            vkCmdBindIndexBuffer(swapchain->cmd_buffs[img_idx], sd->geom_buff_mem->buffer->buffer, pd.vsi.indices_offset, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(swapchain->cmd_buffs[img_idx], pd.vsi.indices_count, 1, 0, 0, 0);
            desc_buff_offset += vtx_pipeline_d_buff->dsl_infos[0].aligned_size;
        }
    }
#endif // DESC_BUFFER
}

void vk_renderer::end_frame()
{
    vkCmdEndRendering(swapchain->cmd_buffs[img_idx]);

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
        .image = swapchain->images[img_idx],
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

    vkCmdPipelineBarrier2(swapchain->cmd_buffs[img_idx], &dep_info);

    vkEndCommandBuffer(swapchain->cmd_buffs[img_idx]);

    const VkSemaphoreSubmitInfo wait_sem_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = acq_sig_sem->semaphore,
            .stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        }
    };

    const VkSemaphoreSubmitInfo sig_sem_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = swapchain->rndr_semaphores[img_idx],
            .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = acq_wait_sem->semaphore,
            .value = ++acq_wait_sem_val,
            .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        }
    };

    const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = swapchain->cmd_buffs[img_idx],
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
        .pFences = &swapchain->present_fences[img_idx],
    };

    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = &present_fence_info,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &swapchain->rndr_semaphores[img_idx],
        .swapchainCount = 1,
        .pSwapchains = &swapchain->swapchain,
        .pImageIndices = &img_idx,
    };

    VK_CHECK("reset present fence", vkResetFences(device->device, 1, &swapchain->present_fences[img_idx]));
    vkQueuePresentKHR(gfx_q, &present_info);
}

void vk_renderer::clear_scene_data()
{
    VK_CHECK("wait for present fence", vkWaitForFences(device->device, 1, &swapchain->present_fences[img_idx], VK_TRUE, UINT64_MAX));
    VK_CHECK("reset present fence", vkResetFences(device->device, 1, &swapchain->present_fences[img_idx]));

    sd = std::make_unique<scene_data>();
}

vk_renderer::~vk_renderer()
{
    VK_CHECK("wait for present fence", vkWaitForFences(device->device, 1, &swapchain->present_fences[img_idx], VK_TRUE, UINT64_MAX));
    VK_CHECK("reset present fence", vkResetFences(device->device, 1, &swapchain->present_fences[img_idx]));

    //VK_CHECK("device wait idle", vkDeviceWaitIdle(device->device));
}
