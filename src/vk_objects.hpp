#pragma once

#include <Volk/volk.h>

#include <Windows.h>

#include <vector>
#include <sstream>
#include <iostream>

#include <SPIRV-Reflect/spirv_reflect.h>

enum CHI_PIPELINE_TYPE
{
    PBR,
    VIEW_AXIS,
};

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

class vk_instance
{
public:
    vk_instance();
    ~vk_instance();

    VkInstance instance;
};

class vk_surface
{
public:
    vk_surface(const VkInstance instance, const HINSTANCE h_instance, const HWND h_wnd);
    ~vk_surface();

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSurfaceCapabilitiesKHR surf_caps = {};
    VkSurfaceFormatKHR format = {};
    VkPresentModeKHR present_mode = {};
    HINSTANCE h_instance = nullptr;
    HWND h_wnd = nullptr;

private:
    VkInstance instance = VK_NULL_HANDLE;
};

class vk_phydev
{
public:
    vk_phydev(const VkInstance instance, vk_surface* surface);

    VkPhysicalDevice phy_dev = VK_NULL_HANDLE;
    uint32_t q_count = 0;
    uint32_t q_fly_idx = 0;
    VkPhysicalDeviceProperties2 props = {};
    VkPhysicalDeviceMemoryProperties mem_props = {};
    VkPhysicalDeviceDescriptorBufferPropertiesEXT desc_buff_props = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT };
};

class vk_device
{
public:
    vk_device(const VkPhysicalDevice phy_dev, const uint32_t q_fly_idx, const uint32_t q_count);
    ~vk_device();

    VkResult wait_semaphores(const std::vector<VkSemaphore>& semaphores, const std::vector<uint64_t>& values)const;

    VkDevice device = VK_NULL_HANDLE;
};

class vk_swapchain
{
public:
    vk_swapchain(const VkDevice device, const vk_surface* surface, const vk_phydev* phy_dev);
    ~vk_swapchain();

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

    uint32_t images_count = 0;
    std::vector<VkImage> images;
    std::vector<VkImageView> image_views;
    VkCommandPool cmd_pool;
    std::vector<VkCommandBuffer> cmd_buffs;
    std::vector<VkSemaphore> rndr_semaphores;
    std::vector<VkFence> present_fences;

private:
    VkDevice device = VK_NULL_HANDLE;
};

class vk_command_pool
{
public:
    vk_command_pool(const VkDevice device, const uint32_t q_fly_idx, const uint32_t cmd_buffs_count);
    ~vk_command_pool();

    VkCommandPool cmd_pool = VK_NULL_HANDLE;

    std::vector<VkCommandBuffer> cmd_buffs;

private:
    VkDevice device = VK_NULL_HANDLE;
};

class vk_command_buffer
{
public:
    vk_command_buffer(const VkDevice device, const VkCommandPool cmd_pool);
    ~vk_command_buffer();

    VkResult begin() const;
    VkResult end() const;

    VkCommandBuffer cmd_buff;

private:
    VkCommandPool cmd_pool;
    VkDevice device;
};

class vk_command_buffers
{
public:
    vk_command_buffers(const VkDevice device);
    ~vk_command_buffers();

    VkResult begin(uint32_t idx)
    {
        const VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        return vkBeginCommandBuffer(cmd_buffs[idx], &begin_info);
    }

    VkResult end(uint32_t idx)
    {
        return vkEndCommandBuffer(cmd_buffs[idx]);
    }

private:
    std::vector<VkCommandBuffer> cmd_buffs;
    VkDevice device = VK_NULL_HANDLE;
};

class vk_semaphore
{
public:
    vk_semaphore(const VkDevice device, bool is_timeline);
    ~vk_semaphore();

    VkSemaphore semaphore;

    VkResult signal(const uint64_t value) const;

    bool is_timeline = false;

private:
    VkDevice device = VK_NULL_HANDLE;
};

class vk_fence
{
public:
    vk_fence(const VkDevice device, const VkBool32 signalled_state);
    ~vk_fence();

    VkFence fence;

private:
    VkDevice device = VK_NULL_HANDLE;
};

class vk_buffer
{
public:
    vk_buffer(const VkDevice device, const VkDeviceSize size, const VkBufferUsageFlags usage);
    ~vk_buffer();

    VkBuffer buffer;

private:
    VkDevice device;
};

class vk_device_memory
{
public:
    vk_device_memory(const VkDevice device, const VkDeviceSize size, const uint32_t type_id);
    ~vk_device_memory();

    VkDeviceMemory memory;

private:
    VkDevice device;
};

class vk_graphics_pipeline
{
public:
    vk_graphics_pipeline(const VkDevice device, const std::string& path, const CHI_PIPELINE_TYPE p_type, const VkFormat format);
    ~vk_graphics_pipeline();

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayout> dsls;

private:
    VkDevice device;
};

class vk_pipeline_layout
{
public:
    vk_pipeline_layout(const VkDevice device) {};
    ~vk_pipeline_layout();

    VkPipelineLayout pipeline_layout;

private:
    VkDevice device;
};

class vk_descriptor_set_layout
{
public:
    vk_descriptor_set_layout() {}
    vk_descriptor_set_layout(const VkDevice device, const SpvReflectDescriptorSet* spv_dsl, const VkShaderStageFlags stage);

    vk_descriptor_set_layout(const vk_descriptor_set_layout& other) = delete;
    vk_descriptor_set_layout& operator=(const vk_descriptor_set_layout& other) = delete;

    vk_descriptor_set_layout(vk_descriptor_set_layout&& other);
    vk_descriptor_set_layout& operator=(vk_descriptor_set_layout&& other);

    ~vk_descriptor_set_layout();

    VkDescriptorSetLayout dsl = VK_NULL_HANDLE;

private:
    VkDevice device;
};

struct host_buffer_memory
{
    host_buffer_memory(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize size, const VkBufferUsageFlags usage);
    host_buffer_memory(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize offset, const VkBufferUsageFlags usage, const std::vector<uint8_t>& data);

    std::unique_ptr<vk_buffer> buffer;
    std::unique_ptr<vk_device_memory> memory;
};

struct device_buffer_memory
{
    device_buffer_memory(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize size, const VkBufferUsageFlags usage);
    device_buffer_memory(const VkDevice device, const VkPhysicalDeviceMemoryProperties& mem_props, const VkDeviceSize offset, const VkBufferUsageFlags usage, const std::vector<uint8_t>& data, const VkQueue xfer_q, const VkCommandBuffer cmd_buff);

    std::unique_ptr<vk_buffer> buffer;
    std::unique_ptr<vk_device_memory> memory;
};
