#pragma once

#include "renderer.hpp"
#include "vk_objects.hpp"

#include <memory>
#include <array>

#define CGLM_FORCE_ZERO_TO_ONE
#include <cglm/include/cglm/cglm.h>

#include <stb/stb_image.h>

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
    uint32_t indices_count = 0;
    uint32_t positions_count = 0;
    uint32_t uvs_count = 0;
    uint32_t nrms_count = 0;

    VkDeviceSize positions_offset = 0;
    VkDeviceSize uvs_offset = 0;
    VkDeviceSize nrms_offset = 0;
    VkDeviceSize indices_offset = 0;

    std::vector<uint8_t> xform_d_buff_desc;
    VkDescriptorBufferInfo xform_d_set_desc = {};
};

struct metal_rough_descriptors
{
    VkDescriptorImageInfo base_color_desc;
    VkDescriptorImageInfo metal_rough_desc;

    VkImage base_color_image;
    VkImage metal_rough_image;

    VkMemoryRequirements2 base_color_mem_reqs;
    VkMemoryRequirements2 metal_rough_mem_reqs;

    float base_color_factors[4];
    float metalness_factor;
    float roughness_factor;

    stbi_uc* base_color_pixels;
    int base_color_width;
    int base_color_height;
    VkDeviceSize base_color_len;
    VkDeviceSize base_color_pixels_offset;

    stbi_uc* metal_rough_pixels;
    int metal_rough_width;
    int metal_rough_height;
    VkDeviceSize metal_rough_len;
    VkDeviceSize metal_rough_pixels_offsets;
};

struct clearcoat_descriptors
{
    VkDescriptorImageInfo cc_desc;
    VkDescriptorImageInfo rough_desc;
    VkDescriptorImageInfo normal_desc;
};

struct material_descriptors
{
    metal_rough_descriptors mtl_rgh_dscs;
    clearcoat_descriptors cc_dsc;
};

struct primitive_data
{
    mesh_shader_info msi;
    vertex_shader_info vsi;

    // primitive level xform desc 
    VkDescriptorBufferInfo xform_desc;
    // primitive level desc pool 
    VkDescriptorPool desc_pool = VK_NULL_HANDLE;
    // primitive level desc set
    VkDescriptorSet desc_set = VK_NULL_HANDLE;
};

struct material_info
{
    uint64_t id;
    material_descriptors mat_dscs;
    std::vector<primitive_data> pds;

    // material level desc pool
    VkDescriptorPool desc_pool = VK_NULL_HANDLE;
    // material level desc set
    VkDescriptorSet desc_set = VK_NULL_HANDLE;
};


namespace scene_data
{
    struct camera
    {
        // map value from the uniform buffer memory;
        void* xform;

        // one descriptor for each swapchain image
        std::vector<VkDescriptorBufferInfo> xform_descs;
        VkDescriptorPool desc_pool;
    };

    struct data
    {
        std::vector<material_info> mis;
        device_buffer_memory::data geom_buff_mem;
        host_buffer_memory::data uni_buff_mem;
        host_buffer_memory::data desc_buff_mem;

        VkImage depth_texture = VK_NULL_HANDLE;
        VkImageView depth_texture_view = VK_NULL_HANDLE;
        VkDeviceMemory depth_texture_memory = VK_NULL_HANDLE;

        VkDeviceMemory images_memory = VK_NULL_HANDLE;

        camera cam;
        // scene level desc pool
        VkDescriptorPool desc_pool = VK_NULL_HANDLE;
        // scene level desc set
        VkDescriptorSet desc_set = VK_NULL_HANDLE;

        vk_raster_pipeline::data mesh_pipeline_d_sets;
        vk_raster_pipeline::data mesh_pipeline_d_buff;
        vk_raster_pipeline::data vtx_pipeline_d_sets;
        vk_raster_pipeline::data vtx_pipeline_d_buff;
    };

    data create(const std::string& file_path, const vk_phydev::data& phy_dev, const vk_surface::data& surface, const uint32_t swapchain_images_count, const VkDevice& device, const VkQueue& xfer_q, const VkCommandBuffer& cmd_buff);
    void destroy(const data d, const VkDevice device);
};

class vk_renderer : public renderer
{
public:
    vk_renderer(const HWND h_wnd);

    void import_scene_data(const std::string& file_path) override;
    void resize(const uint32_t width, const uint32_t height) override;
    void begin_frame() override;
    void clear_frame(const float color[]) override;
    void render_world(const mat4 cam_xform) override;
    void end_frame() override;
    void clear_scene_data() override;
    void render_offline(const uint32_t render_width, const uint32_t render_height, uint8_t* pixels) override;

    ~vk_renderer();

private:
    VkInstance instance;
    vk_surface::data surface;
    vk_phydev::data phy_dev;
    VkDevice device;
    vk_swapchain::data swapchain;
    vk_semaphore::data acq_sig_sem;
    vk_semaphore::data acq_wait_sem;

    vk_command_pool::data xfer_cmd_pool;

    VkQueue gfx_q;
    VkQueue xfer_q;

    uint64_t acq_wait_sem_val;
    VkViewport viewport;

    scene_data::data sd;
    // current image index of the swapchain
    uint32_t img_idx;
};