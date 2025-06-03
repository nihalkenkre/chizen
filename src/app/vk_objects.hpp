#pragma once

//#include <Volk/volk.h>

#include <Windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include <vector>
#include <sstream>
#include <iostream>

#include <SPIRV-Reflect/spirv_reflect.h>

#include <cgltf.h>

//#define DESC_BUFFER

enum CHI_PIPELINE_TYPE
{
    VERTEX,
    MESH,
};

namespace func_ptrs
{
   extern "C" PFN_vkSetDebugUtilsObjectNameEXT vk_SetDebugUtilsObjectNameEXT;
   extern "C" PFN_vkCreateRayTracingPipelinesKHR vk_CreateRayTracingPipelinesKHR;
   extern "C" PFN_vkGetAccelerationStructureBuildSizesKHR vk_GetAccelerationStructureBuildSizesKHR;
   extern "C" PFN_vkCreateAccelerationStructureKHR vk_CreateAccelerationStructureKHR;
   extern "C" PFN_vkDestroyAccelerationStructureKHR vk_DestroyAccelerationStructureKHR;
}

uint32_t get_memory_type_id(const VkPhysicalDeviceMemoryProperties2 mem_props, const VkMemoryRequirements2 mem_reqs, const uint32_t mem_prop_types);
void copy_buffer_to_buffer(const VkBuffer src_buffer, const VkBuffer dst_buffer, const std::vector<VkBufferCopy> regions, const VkCommandBuffer cmd_buff, const VkQueue xfer_q);
void copy_buffer_to_image(const VkBuffer src_buffer, const VkImage dst_image, const VkImageLayout dst_image_layout, const std::vector<VkBufferImageCopy2>& regions, const VkCommandBuffer cmd_buff, const VkQueue xfer_q);

static inline void VK_CHECK(const std::string& action, const VkResult result)
{
    if (result < VK_SUCCESS)
    {
        std::stringstream msg;
        msg << "ERR: " << action << " " << result << "\nExiting...\n";
#ifdef DEBUG
        OutputDebugStringA(msg.str().c_str());
#else
        std::cout << msg.str();
#endif
        std::exit(result);
    }
}

static inline void SPV_CHECK(const std::string& action, const SpvReflectResult result)
{
    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        std::stringstream msg;
        msg << "ERR: " << action << " " << result << "\nExiting...\n";
#ifdef DEBUG
        OutputDebugStringA(msg.str().c_str());
#else
        std::cout << msg.str();
#endif
        std::exit(result);
    }
}

namespace vk_instance
{
    VkInstance create();
    void destroy(const VkInstance instance);
}

//class vk_instance
//{
//public:
//    vk_instance();
//    ~vk_instance();
//
//    vk_instance(const vk_instance& other) = delete;
//    vk_instance& operator=(const vk_instance& other) = delete;
//
//    vk_instance(vk_instance&& other)
//    {
//        this->instance = other.instance;
//
//        other.instance = VK_NULL_HANDLE;
//    }
//
//    vk_instance& operator=(vk_instance&& other)
//    {
//        this->instance = other.instance;
//
//        other.instance = VK_NULL_HANDLE;
//
//        return *this;
//    }
//
//    VkInstance instance = VK_NULL_HANDLE;
//};

namespace vk_surface
{
    struct data
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkSurfaceCapabilitiesKHR surf_caps = {};
        VkPresentModeKHR present_mode = {};
        VkSurfaceFormatKHR format = {};
        HINSTANCE h_instance = nullptr;
        HWND h_wnd = nullptr;
    };

    data create(const VkInstance instnce, const HINSTANCE h_instance, const HWND h_wnd);
    void destroy(const VkSurfaceKHR surface, VkInstance instance);
}

//class vk_surface
//{
//public:
//    vk_surface() {};
//    vk_surface(const VkInstance instance, const HINSTANCE h_instance, const HWND h_wnd);
//    ~vk_surface();
//
//    VkSurfaceKHR surface = VK_NULL_HANDLE;
//    VkSurfaceCapabilitiesKHR surf_caps = {};
//    VkSurfaceFormatKHR format = {};
//    VkPresentModeKHR present_mode = {};
//    HINSTANCE h_instance = nullptr;
//    HWND h_wnd = nullptr;
//
//    vk_surface(const vk_surface& other) = delete;
//    vk_surface& operator=(const vk_surface& other) = delete;
//
//    vk_surface(vk_surface&& other)
//    {
//        surface = other.surface;
//        instance = other.instance;
//        std::memcpy(&surf_caps, &other.surf_caps, sizeof(other.surf_caps));
//        std::memcpy(&format, &other.format, sizeof(other.format));
//        std::memcpy(&present_mode, &other.present_mode, sizeof(other.present_mode));
//        h_instance = other.h_instance;
//        h_wnd = other.h_wnd;
//
//        other.surface = VK_NULL_HANDLE;
//        other.instance = VK_NULL_HANDLE;
//        other.surf_caps = {};
//        other.format = {};
//        other.present_mode = {};
//        h_instance = nullptr;
//        h_wnd = nullptr;
//    }
//
//    vk_surface& operator=(vk_surface&& other)
//    {
//        surface = other.surface;
//        instance = other.instance;
//        std::memcpy(&surf_caps, &other.surf_caps, sizeof(other.surf_caps));
//        std::memcpy(&format, &other.format, sizeof(other.format));
//        std::memcpy(&present_mode, &other.present_mode, sizeof(other.present_mode));
//        h_instance = other.h_instance;
//        h_wnd = other.h_wnd;
//
//        other.surface = VK_NULL_HANDLE;
//        other.instance = VK_NULL_HANDLE;
//        other.surf_caps = {};
//        other.format = {};
//        other.present_mode = {};
//        h_instance = nullptr;
//        h_wnd = nullptr;
//
//        return *this;
//    }
//
//private:
//    VkInstance instance = VK_NULL_HANDLE;
//};

//class vk_phy_dev
//{
//public:
//    vk_phy_dev() {}
//    vk_phy_dev(const VkInstance instance, vk_surface::data* surface);
//
//    VkPhysicalDevice phy_dev = VK_NULL_HANDLE;
//    uint32_t q_count = 0;
//    uint32_t q_fly_idx = 0;
//    VkPhysicalDeviceProperties2 props = {};
//    VkPhysicalDeviceMemoryProperties mem_props = {};
//    // Setting .sType so let vkGetPhysicalDeviceFeatures know what struct this is
//    VkPhysicalDeviceDescriptorBufferPropertiesEXT desc_buff_props = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT };
//};

namespace vk_phydev
{
    struct data
    {
        VkPhysicalDevice phy_dev = VK_NULL_HANDLE;
        uint32_t q_count = 0;
        uint32_t q_fly_idx = 0;
        VkPhysicalDeviceProperties2 props = {};
        VkPhysicalDeviceMemoryProperties2 mem_props = {};
        VkPhysicalDeviceDescriptorBufferPropertiesEXT desc_buff_props = {};
    };

    data get_phy_dev(const VkInstance instance, vk_surface::data* surface);
    data get_phy_dev(const VkInstance instance);
}

//class vk_device
//{
//public:
//    vk_device() {}
//    vk_device(const VkPhysicalDevice phy_dev, const uint32_t q_fly_idx, const uint32_t q_count);
//    ~vk_device();
//
//    vk_device(const vk_device& other) = delete;
//    vk_device& operator=(const vk_device& other) = delete;
//
//    vk_device(vk_device&& other)
//    {
//        device = other.device;
//        other.device = VK_NULL_HANDLE;
//    }
//
//    vk_device& operator=(vk_device&& other)
//    {
//        device = other.device;
//        other.device = VK_NULL_HANDLE;
//
//        return *this;
//    }
//
//    VkDevice device = VK_NULL_HANDLE;
//};

namespace vk_device
{
    VkDevice create(const VkPhysicalDevice phy_dev, const uint32_t q_fly_idx, const uint32_t q_count);
    void destroy(const VkDevice device);
}

//class vk_swapchain
//{
//public:
//    vk_swapchain() {};
//    vk_swapchain(const VkDevice device, const vk_surface::data& surface, const vk_phy_dev::data& phy_dev);
//    ~vk_swapchain();
//
//    vk_swapchain(const vk_swapchain& other) = delete;
//    vk_swapchain& operator=(const vk_swapchain& other) = delete;
//
//    vk_swapchain(vk_swapchain&& other)
//    {
//        swapchain = other.swapchain;
//        device = other.device;
//
//        images_count = other.images_count;
//        images = std::move(other.images);
//        image_views = std::move(other.image_views);
//        cmd_pool = other.cmd_pool;
//        cmd_buffs = std::move(other.cmd_buffs);
//        rndr_semaphores = std::move(other.rndr_semaphores);
//        present_fences = std::move(other.present_fences);
//
//        other.swapchain = VK_NULL_HANDLE;
//        other.device = other.device;
//        other.images_count = 0;
//        other.cmd_pool = VK_NULL_HANDLE;
//    }
//
//    vk_swapchain& operator=(vk_swapchain&& other)
//    {
//        swapchain = other.swapchain;
//        device = other.device;
//
//        images_count = other.images_count;
//        images = std::move(other.images);
//        image_views = std::move(other.image_views);
//        cmd_pool = other.cmd_pool;
//        cmd_buffs = std::move(other.cmd_buffs);
//        rndr_semaphores = std::move(other.rndr_semaphores);
//        present_fences = std::move(other.present_fences);
//
//        other.swapchain = VK_NULL_HANDLE;
//        other.device = other.device;
//        other.images_count = 0;
//        other.cmd_pool = VK_NULL_HANDLE;
//
//        return *this;
//    }
//
//    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
//
//    VkCommandPool cmd_pool = VK_NULL_HANDLE;
//    uint32_t images_count = 0;
//    std::vector<VkImage> images;
//    std::vector<VkImageView> image_views;
//    std::vector<VkCommandBuffer> cmd_buffs;
//    std::vector<VkSemaphore> rndr_semaphores;
//    std::vector<VkFence> present_fences;
//
//private:
//    VkDevice device = VK_NULL_HANDLE;
//};

namespace vk_swapchain
{
    struct data
    {
        VkSwapchainKHR swapchain;
        VkCommandPool cmd_pool;

        std::vector<VkImage> images;
        std::vector<VkImageView> image_views;
        std::vector<VkCommandBuffer> cmd_buffs;
        std::vector<VkSemaphore> rndr_semaphores;
        std::vector<VkFence> present_fences;
        uint32_t images_count;
    };

    vk_swapchain::data create(const VkDevice device, const vk_surface::data& surface, const vk_phydev::data& phy_dev, const std::string& name);
    void destroy(vk_swapchain::data data, const VkDevice device);
}

//class vk_command_pool
//{
//public:
//    vk_command_pool() {}
//    vk_command_pool(const VkDevice device, const uint32_t q_fly_idx, const uint32_t cmd_buffs_count);
//    ~vk_command_pool();
//
//    vk_command_pool(const vk_command_pool& other) = delete;
//    vk_command_pool& operator= (const vk_command_pool& other) = delete;
//
//    vk_command_pool(vk_command_pool&& other)
//    {
//        cmd_pool = other.cmd_pool;
//        device = other.device;
//        cmd_buffs = std::move(other.cmd_buffs);
//
//        other.cmd_pool = VK_NULL_HANDLE;
//        other.device = VK_NULL_HANDLE;
//    }
//
//    vk_command_pool& operator=(vk_command_pool&& other)
//    {
//        cmd_pool = other.cmd_pool;
//        device = other.device;
//        cmd_buffs = std::move(other.cmd_buffs);
//
//        other.cmd_pool = VK_NULL_HANDLE;
//        other.device = VK_NULL_HANDLE;
//
//        return *this;
//    }
//
//    VkCommandPool cmd_pool = VK_NULL_HANDLE;
//    std::vector<VkCommandBuffer> cmd_buffs;
//
//private:
//    VkDevice device = VK_NULL_HANDLE;
//};

namespace vk_command_pool
{
    struct data {
        VkCommandPool cmd_pool;
        std::vector<VkCommandBuffer> cmd_buffs;
    };

    data create(const VkDevice device, const uint32_t q_fly_idx, const uint32_t cmd_buffs_count, const std::string& name);
    void destroy(vk_command_pool::data cmd_pool, const VkDevice device);
}

//class vk_command_buffer
//{
//public:
//    vk_command_buffer() {}
//    vk_command_buffer(const VkDevice device, const VkCommandPool cmd_pool, const std::string& name);
//
//    VkCommandBuffer cmd_buff;
//
//private:
//    VkCommandPool cmd_pool;
//    VkDevice device;
//};

namespace vk_command_buffer
{
    VkCommandBuffer allocate(const VkDevice device, const VkCommandPool cmd_pool, const std::string& name);
}

//class vk_semaphore
//{
//public:
//    vk_semaphore() {}
//    vk_semaphore(const VkDevice device, bool is_timeline);
//    ~vk_semaphore();
//
//    vk_semaphore(const vk_semaphore& other) = delete;
//    vk_semaphore& operator=(const vk_semaphore& other) = delete;
//
//    vk_semaphore(vk_semaphore&& other)
//    {
//        is_timeline = other.is_timeline;
//        semaphore = other.semaphore;
//        device = other.device;
//
//        other.is_timeline = false;
//        other.semaphore = VK_NULL_HANDLE;
//        other.device = VK_NULL_HANDLE;
//    }
//
//    vk_semaphore& operator=(vk_semaphore&& other)
//    {
//        is_timeline = other.is_timeline;
//        semaphore = other.semaphore;
//        device = other.device;
//
//        other.is_timeline = false;
//        other.semaphore = VK_NULL_HANDLE;
//        other.device = VK_NULL_HANDLE;
//
//        return *this;
//    }
//
//    VkSemaphore semaphore;
//
//    bool is_timeline = false;
//
//private:
//    VkDevice device = VK_NULL_HANDLE;
//};

namespace vk_semaphore
{
    struct data
    {
        VkSemaphore semaphore;
        VkSemaphoreType type;
    };

    vk_semaphore::data create(const VkDevice device, const VkSemaphoreType semaphore_type, const std::string& name);
    void destroy(const VkSemaphore semaphore, const VkDevice device);
}

//class vk_fence
//{
//public:
//    vk_fence() {}
//    vk_fence(const VkDevice device, const VkBool32 signalled_state);
//    ~vk_fence();
//
//    vk_fence(const vk_fence& other) = delete;
//    vk_fence& operator=(const vk_fence& other) = delete;
//
//    vk_fence(vk_fence&& other)
//    {
//        fence = other.fence;
//        device = other.device;
//
//        other.fence = VK_NULL_HANDLE;
//        other.device = VK_NULL_HANDLE;
//    }
//
//    vk_fence& operator=(vk_fence&& other)
//    {
//        fence = other.fence;
//        device = other.device;
//
//        other.fence = VK_NULL_HANDLE;
//        other.device = VK_NULL_HANDLE;
//
//        return *this;
//    }
//
//    VkFence fence;
//
//private:
//    VkDevice device = VK_NULL_HANDLE;
//};

namespace vk_fence
{
    VkFence create(const VkDevice device, const VkFenceCreateFlags flags, const std::string& name);
    void destroy(const VkFence fence, const VkDevice device);
}

//class vk_buffer
//{
//public:
//    vk_buffer() {}
//    vk_buffer(const VkDevice device, const VkDeviceSize size, const VkBufferUsageFlags usage);
//    ~vk_buffer();
//
//    vk_buffer(const vk_buffer& other) = delete;
//    vk_buffer& operator=(const vk_buffer& other) = delete;
//
//    vk_buffer(vk_buffer&& other)
//    {
//        buffer = other.buffer;
//        device = other.device;
//
//        other.buffer = VK_NULL_HANDLE;
//        other.device = VK_NULL_HANDLE;
//    }
//
//    vk_buffer& operator=(vk_buffer&& other)
//    {
//        buffer = other.buffer;
//        device = other.device;
//
//        other.buffer = VK_NULL_HANDLE;
//        other.device = VK_NULL_HANDLE;
//
//        return *this;
//    }
//
//    VkBuffer buffer = VK_NULL_HANDLE;
//
//private:
//    VkDevice device = VK_NULL_HANDLE;
//};

namespace vk_buffer
{
    VkBuffer create(const VkDevice device, const VkDeviceSize size, const VkBufferUsageFlags usage, const std::string& name);
    void destroy(const VkBuffer buffer, const VkDevice device);
}

//class vk_device_memory
//{
//public:
//    vk_device_memory() {}
//    vk_device_memory(const VkDevice device, const VkDeviceSize size, const uint32_t type_id);
//    vk_device_memory(const VkDevice device, const VkDeviceSize size, const uint32_t type_id, const VkMemoryAllocateFlagsInfo& flags_info);
//    ~vk_device_memory();
//
//    vk_device_memory(const vk_device_memory& other) = delete;
//    vk_device_memory& operator=(const vk_device_memory& other) = delete;
//
//    vk_device_memory(vk_device_memory&& other)
//    {
//        memory = other.memory;
//        device = other.device;
//
//        other.memory = VK_NULL_HANDLE;
//        other.device = VK_NULL_HANDLE;
//    }
//
//    vk_device_memory& operator=(vk_device_memory&& other)
//    {
//        memory = other.memory;
//        device = other.device;
//
//        other.memory = VK_NULL_HANDLE;
//        other.device = VK_NULL_HANDLE;
//
//        return *this;
//    }
//
//    VkDeviceMemory memory = VK_NULL_HANDLE;
//
//private:
//    VkDevice device = VK_NULL_HANDLE;
//};

namespace vk_device_memory
{
    VkDeviceMemory allocate(const VkDevice device, const VkDeviceSize size, const uint32_t type_id, const std::string& name);
    VkDeviceMemory allocate(const VkDevice device, const VkDeviceSize size, const uint32_t type_id, const VkMemoryAllocateFlagsInfo& flags_info, const std::string& name);
    void free(const VkDeviceMemory memory, const VkDevice device);
}

struct vk_descriptor_binding
{
    // offset for descriptor buffer
    VkDeviceSize offset;
};

//class vk_descriptor_set_layout
//{
//public:
//    vk_descriptor_set_layout() {}
//    //vk_descriptor_set_layout(const VkDevice device, const SpvReflectDescriptorSet* spv_dsl, const VkShaderStageFlags stage, const VkDeviceSize alignment);
//
//    vk_descriptor_set_layout(const vk_descriptor_set_layout& other) = delete;
//    vk_descriptor_set_layout& operator=(const vk_descriptor_set_layout& other) = delete;
//
//    vk_descriptor_set_layout(vk_descriptor_set_layout&& other);
//    vk_descriptor_set_layout& operator=(vk_descriptor_set_layout&& other);
//
//    ~vk_descriptor_set_layout();
//
//    VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
//    // Size for descriptor buffer
//    VkDeviceSize size;
//
//    std::vector<vk_descriptor_binding> bindings;
//
//private:
//    VkDevice device;
//};

struct dsl_binding_info
{
    // Offset for desc buffer
    VkDeviceSize offset;
    // Type of the descriptor;
    VkDescriptorType type;
};

struct dsl_info
{
    // Aligned size of the desc set layout
    VkDeviceSize aligned_size;
    std::vector<dsl_binding_info> binding_infos;
};

//class vk_graphics_pipeline
//{
//public:
//    vk_graphics_pipeline() {}
//    vk_graphics_pipeline(const VkDevice device, const std::string& path, const CHI_PIPELINE_TYPE& p_type, const VkFormat format, const VkPhysicalDeviceDescriptorBufferPropertiesEXT& desc_buff_props);
//    ~vk_graphics_pipeline();
//
//    vk_graphics_pipeline(const vk_graphics_pipeline& other) = delete;
//    vk_graphics_pipeline& operator=(const vk_graphics_pipeline& other) = delete;
//
//    vk_graphics_pipeline(vk_graphics_pipeline&& other)
//    {
//        pipeline = other.pipeline;
//        pipeline_layout = other.pipeline_layout;
//        device = other.device;
//        dsl_infos = std::move(other.dsl_infos);
//        dsls = std::move(other.dsls);
//        p_type = other.p_type;
//
//        other.pipeline = VK_NULL_HANDLE;
//        other.pipeline_layout = VK_NULL_HANDLE;
//        other.device = VK_NULL_HANDLE;
//    }
//
//    vk_graphics_pipeline& operator=(vk_graphics_pipeline&& other)
//    {
//        pipeline = other.pipeline;
//        pipeline_layout = other.pipeline_layout;
//        device = other.device;
//        dsl_infos = std::move(other.dsl_infos);
//        dsls = std::move(other.dsls);
//        p_type = other.p_type;
//
//        other.pipeline = VK_NULL_HANDLE;
//        other.pipeline_layout = VK_NULL_HANDLE;
//        other.device = VK_NULL_HANDLE;
//
//        return *this;
//    }
//
//    VkPipeline pipeline = VK_NULL_HANDLE;
//    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
//
//    std::vector<dsl_info> dsl_infos;
//    std::vector<VkDescriptorSetLayout> dsls;
//
//    CHI_PIPELINE_TYPE p_type = CHI_PIPELINE_TYPE::VERTEX;
//private:
//    VkDevice device;
//};

namespace vk_raster_pipeline
{
    struct data
    {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;

        std::vector<dsl_info> dsl_infos;
        std::vector<VkDescriptorSetLayout> dsls;

        CHI_PIPELINE_TYPE p_type = CHI_PIPELINE_TYPE::VERTEX;
    };

    vk_raster_pipeline::data create(const VkDevice device, const std::string& path, const CHI_PIPELINE_TYPE& p_type, const VkFormat format, const VkPhysicalDeviceDescriptorBufferPropertiesEXT& desc_buff_props, const std::string& name);
    void destroy(const data d, const VkDevice device);
}

//class vk_descriptor_pool
//{
//public:
//    vk_descriptor_pool() {}
//    vk_descriptor_pool(const VkDevice& device, const uint32_t& max_sets, const std::vector<VkDescriptorPoolSize>& pool_sizes);
//    ~vk_descriptor_pool();
//
//    vk_descriptor_pool(const vk_descriptor_pool& other) = delete;
//    vk_descriptor_pool& operator=(const vk_descriptor_pool& other) = delete;
//
//    vk_descriptor_pool(vk_descriptor_pool&& other)
//    {
//        descriptor_pool = other.descriptor_pool;
//        device = other.device;
//
//        other.descriptor_pool = VK_NULL_HANDLE;
//        other.device = VK_NULL_HANDLE;
//    }
//
//    vk_descriptor_pool& operator=(vk_descriptor_pool&& other)
//    {
//        descriptor_pool = other.descriptor_pool;
//        device = other.device;
//
//        other.descriptor_pool = VK_NULL_HANDLE;
//        other.device = VK_NULL_HANDLE;
//
//        return *this;
//    }
//
//    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
//
//private:
//    VkDevice device = VK_NULL_HANDLE;
//};

namespace vk_descriptor_pool
{
    VkDescriptorPool create(const VkDevice& device, const uint32_t& max_sets, const std::vector<VkDescriptorPoolSize>& pool_sizes, const std::string& name);
    void destroy(const VkDescriptorPool desc_pool, const VkDevice device);
}

//class vk_descriptor_sets
//{
//public:
//    vk_descriptor_sets() {}
//    vk_descriptor_sets(const VkDevice& device, const VkDescriptorPool& desc_pool, const std::vector<VkDescriptorSetLayout>& desc_set_layouts);
//
//    std::vector<VkDescriptorSet> desc_sets;
//
//private:
//    VkDevice device = VK_NULL_HANDLE;
//};

namespace vk_descriptor_sets
{
    std::vector<VkDescriptorSet> allocate(const VkDevice& device, const VkDescriptorPool& desc_pool, const std::vector<VkDescriptorSetLayout>& desc_set_layouts, const std::string& name);
}

//class vk_image
//{
//public:
//    vk_image(const VkDevice& device, const VkExtent3D& extent, const VkFormat& format, const VkImageUsageFlags& usage);
//    ~vk_image();
//
//    VkImage image = VK_NULL_HANDLE;
//
//private:
//    VkDevice device = VK_NULL_HANDLE;
//};

namespace vk_image
{
    VkImage create(const VkDevice device, const VkExtent3D& extent, const VkFormat format, const VkImageUsageFlags usage, const std::string& name);
    void destroy(const VkImage image, const VkDevice device);
}

namespace vk_image_view
{
    VkImageView create(const VkDevice device, const VkImage image, const VkImageViewType view_type, const VkFormat format, const VkImageAspectFlags aspect_mask, const std::string& name);
    void destroy(const VkImageView image_view, const VkDevice device);
};

namespace vk_sampler
{
    VkSampler create(const VkDevice device, const cgltf_filter_type min_filter, const cgltf_filter_type mag_filter, const cgltf_wrap_mode wrap_s, const cgltf_wrap_mode wrap_t, const float& max_anisotropy, const float min_lod, const float max_lod, const std::string& name);
    void destroy(const VkSampler sampler, const VkDevice device);
}

//struct host_buffer_memory
//{
//    host_buffer_memory() {}
//    host_buffer_memory(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize size, const VkBufferUsageFlags usage);
//    host_buffer_memory(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize offset, const VkBufferUsageFlags usage, const std::vector<uint8_t>& data);
//
//    host_buffer_memory(const host_buffer_memory& other) = delete;
//    host_buffer_memory& operator=(const host_buffer_memory& other) = delete;
//
//    host_buffer_memory(host_buffer_memory&& other)
//    {
//        buffer = std::move(other.buffer);
//        memory = std::move(other.memory);
//        addr = other.addr;
//        usage = other.usage;
//
//        other.addr = 0;
//        other.usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
//    }
//
//    host_buffer_memory& operator=(host_buffer_memory&& other)
//    {
//        buffer = std::move(other.buffer);
//        memory = std::move(other.memory);
//        addr = other.addr;
//        usage = other.usage;
//
//        other.addr = 0;
//        other.usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
//
//        return *this;
//    }
//
//    vk_buffer buffer;
//    vk_device_memory memory;
//
//    // Only valid for buffers with VK**SHADER_ADDRESS_BIT
//    VkDeviceAddress addr = 0;
//    VkBufferUsageFlags usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
//
//    void* map = nullptr;
//};

namespace host_buffer_memory
{
    struct data
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;

        // Only valid for buffers with VK**SHADER_ADDRESS_BIT
        VkDeviceAddress addr = 0;
        void* map = nullptr;

        VkBufferUsageFlags usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
        VkDeviceSize size = 0;
    };

    data create(const VkDevice device, const VkPhysicalDeviceMemoryProperties2& mem_props, const VkDeviceSize offset, const VkBufferUsageFlags usage, const std::vector<uint8_t>& data, const std::string& name);
    data create(const VkDevice device, const VkPhysicalDeviceMemoryProperties2& mem_props, const VkDeviceSize size, const VkBufferUsageFlags usage, const std::string& name);

    void destroy(const data bm, const VkDevice device);
}

//struct device_buffer_memory
//{
//    device_buffer_memory() {}
//    device_buffer_memory(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize size, const VkBufferUsageFlags usage);
//    device_buffer_memory(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize offset, VkBufferUsageFlags usage, const std::vector<uint8_t>& data, const VkQueue xfer_q, const VkCommandBuffer cmd_buff);
//
//    device_buffer_memory(const vk_device_memory& other) = delete;
//    device_buffer_memory& operator=(const vk_device_memory& other) = delete;
//
//    device_buffer_memory(device_buffer_memory&& other)
//    {
//        buffer = std::move(other.buffer);
//        memory = std::move(other.memory);
//        addr = other.addr;
//        usage = other.usage;
//
//        other.addr = 0;
//        other.usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
//    }
//
//    device_buffer_memory& operator=(device_buffer_memory&& other)
//    {
//        buffer = std::move(other.buffer);
//        memory = std::move(other.memory);
//        addr = other.addr;
//        usage = other.usage;
//
//        other.addr = 0;
//        other.usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
//
//        return *this;
//    }
//
//    vk_buffer buffer;
//    vk_device_memory memory;
//
//    // Only valid for buffers with VK**SHADER_ADDRESS_BIT
//    VkDeviceAddress addr = 0;
//    VkBufferUsageFlags usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
//};

namespace device_buffer_memory
{
    struct data
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;

        // Only valid for buffers with VK**SHADER_DEVICE_ADDRESS_BIT
        VkDeviceAddress addr = 0;
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
    };

    device_buffer_memory::data create(const VkDevice device, const VkPhysicalDeviceMemoryProperties2& mem_props, const VkDeviceSize size, const VkBufferUsageFlags usage, const std::string& name);
    device_buffer_memory::data create(const VkDevice device, const VkPhysicalDeviceMemoryProperties2& mem_props, const VkDeviceSize offset, VkBufferUsageFlags usage, const std::vector<uint8_t>& data, const VkQueue xfer_q, const VkCommandBuffer cmd_buff, const std::string& name);

    void destroy(const data bm, const VkDevice device);
}

namespace vk_desc_img_info
{
    struct data
    {
        VkDescriptorImageInfo desc;
        VkImage image;
    };

    data create(const VkDevice device);
    void destroy(const data image_info, const VkDevice device);
}
