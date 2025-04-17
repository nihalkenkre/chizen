#pragma once

#include "renderer.hpp"
#include "vk_objects.hpp"

#include <memory>
#include <array>

#include <cglm/include/cglm/cglm.h>

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
    std::unique_ptr<vk_graphics_pipeline> pbr_pipeline;

    std::unique_ptr<vk_command_pool> xfer_cmd_pool;

    VkQueue gfx_q;
    VkQueue xfer_q;

    uint32_t img_idx;
    uint64_t acq_wait_sem_val;
    VkViewport viewport;

    struct float3
    {
        float x;
        float y;
        float z;
    };

    struct primitive_data
    {
        // 1 desc for each swapchain image
        std::vector<VkDescriptorBufferInfo> xform_descs;
        // 4 descs for positions, meshlets, meshlets_verts, meshlets_tris
        std::array<VkDescriptorBufferInfo, 4> geom_descs;
        size_t meshlets_count;
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
        // one buffer for all the uniform data for all the swapchain images
        std::unique_ptr<host_buffer_memory> uni_buff_mem;
        std::vector<vk_descriptor_set_layout> dsls;
        std::vector<vk_pipeline_layout> pls;
        std::vector<vk_graphics_pipeline> gps;
    };

    std::unique_ptr<scene_data> sd;
};