#pragma once

#include "renderer.hpp"
#include "vk_objects.hpp"

#include <memory>
#include <array>

#include <cglm/include/cglm/cglm.h>


struct float3
{
    float x;
    float y;
    float z;
};

struct mesh_shader_info
{
    std::array<VkDeviceSize, 4> geom_data_sizes;
    std::array<VkDeviceSize, 4> geom_data_offsets;
    // 4 descs for positions, meshlets, meshlets_verts, meshlets_tris
    std::array<VkDeviceSize, 4> geom_descs_offsets;
    size_t meshlets_count;

    VkDescriptorBufferInfo xform_d_set_desc;
    VkDescriptorBufferInfo positions_d_set_desc;
    VkDescriptorBufferInfo meshlets_d_set_desc;
    VkDescriptorBufferInfo meshlets_vtx_d_set_desc;
    VkDescriptorBufferInfo meshlets_tri_d_set_desc;

    std::vector<uint8_t> xform_d_buff_desc;
};

struct vertex_shader_info
{
    VkDeviceSize positions_offset = 0;
    VkDeviceSize uvs_offsets = 0;
    VkDeviceSize indices_offset = 0;
    uint32_t indices_count = 0;

    std::vector<uint8_t> xform_d_buff_desc;
    VkDescriptorBufferInfo xform_d_set_desc = {};
};

struct metal_rough_descriptors
{
    VkDescriptorImageInfo base_color_desc;
    VkDescriptorImageInfo metal_rough_desc;

    float base_color_factors[4];
    float metalness_factor;
    float roughness_factor;
};

struct clearcoat_descriptors
{
    VkDescriptorImageInfo cc_desc;
    VkDescriptorImageInfo rough_desc;
    VkDescriptorImageInfo normal_desc;
};

struct material_descriptors
{
    metal_rough_descriptors met_rough_dscs;
    clearcoat_descriptors cc_dsc;
};

struct primitive_data
{
    mesh_shader_info msi;
    vertex_shader_info vsi;

    std::vector<uint32_t> indices;
    std::vector<uint8_t> positions;
    std::vector<uint8_t> uvs;

    material_descriptors mat_dscs;

    VkDescriptorPool desc_pool = VK_NULL_HANDLE;
    VkDescriptorSet desc_set = VK_NULL_HANDLE;
};

struct material_info
{
    uint64_t id;
    std::vector<primitive_data> pds;
};

namespace scene_data
{
    struct data
    {
        std::vector<material_info> mis;
        device_buffer_memory::data geom_buff_mem;
        host_buffer_memory::data uni_buff_mem;
        host_buffer_memory::data desc_buff_mem;

        std::vector<uint8_t> cam_xform_d_buff_desc;
        VkDescriptorBufferInfo cam_xform_d_set_desc = {};

        VkDescriptorPool desc_pool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> desc_sets;

        vk_graphics_pipeline::data mesh_pipeline_d_sets;
        vk_graphics_pipeline::data mesh_pipeline_d_buff;
        vk_graphics_pipeline::data vtx_pipeline_d_sets;
        vk_graphics_pipeline::data vtx_pipeline_d_buff;
    };

    data create(const std::string& file_path, const vk_phy_dev::data& phy_dev, const vk_surface::data& surface, const VkDevice& device, const VkQueue& xfer_q, const VkCommandBuffer& cmd_buff);
    void destroy(const data d, const VkDevice device);
};

class vk_renderer : public renderer
{
public:
    vk_renderer(const HWND h_wnd);

    void import_scene_data(const std::string& file_path) override;
    void resize(const UINT width, const UINT height) override;
    void begin_frame() override;
    void clear_frame(const float color[]) override;
    void render_world() override;
    void end_frame() override;
    void clear_scene_data() override;

    ~vk_renderer();

private:
    VkInstance instance;
    vk_surface::data surface;
    vk_phy_dev::data phy_dev;
    VkDevice device;
    vk_swapchain::data swapchain;
    vk_semaphore::data acq_sig_sem;
    vk_semaphore::data acq_wait_sem;

    vk_command_pool::data xfer_cmd_pool;

    VkQueue gfx_q;
    VkQueue xfer_q;

    uint32_t img_idx;
    uint64_t acq_wait_sem_val;
    VkViewport viewport;

    scene_data::data sd;
};