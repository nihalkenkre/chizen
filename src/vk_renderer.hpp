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
    VkDeviceSize indices_offset = 0;
    uint32_t indices_count = 0;

    std::vector<uint8_t> xform_d_buff_desc;
    VkDescriptorBufferInfo xform_d_set_desc = {};
};

struct primitive_data
{
    mesh_shader_info msi;
    vertex_shader_info vsi;
};

struct material_info
{
    uint64_t id;
    std::vector<primitive_data> pds;
};

struct scene_data
{
    std::vector<material_info> mis;
    std::unique_ptr<device_buffer_memory> geom_buff_mem;
    std::unique_ptr<host_buffer_memory> uni_buff_mem;
    std::unique_ptr<host_buffer_memory> desc_buff_mem;

    std::vector<uint8_t> cam_xform_d_buff_desc;
    VkDescriptorBufferInfo cam_xform_d_set_desc = {};

    std::unique_ptr<vk_descriptor_pool> desc_pool;
    std::unique_ptr<vk_descriptor_sets> desc_sets;
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
    std::unique_ptr<vk_instance> instance;
    std::unique_ptr<vk_surface> surface;
    std::unique_ptr<vk_phydev> phy_dev;
    std::unique_ptr<vk_device> device;
    std::unique_ptr<vk_swapchain> swapchain;
    std::unique_ptr<vk_semaphore> acq_sig_sem;
    std::unique_ptr<vk_semaphore> acq_wait_sem;

    std::unique_ptr<vk_graphics_pipeline> mesh_d_sets_pipeline;
    std::unique_ptr<vk_graphics_pipeline> mesh_d_buff_pipeline;
    std::unique_ptr<vk_graphics_pipeline> vtx_pipeline_d_sets;
    std::unique_ptr<vk_graphics_pipeline> vtx_pipeline_d_buff;

    std::unique_ptr<vk_command_pool> xfer_cmd_pool;

    VkQueue gfx_q;
    VkQueue xfer_q;

    uint32_t img_idx;
    uint64_t acq_wait_sem_val;
    VkViewport viewport;

    std::unique_ptr<scene_data> sd;
};