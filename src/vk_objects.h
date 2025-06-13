#pragma once

#include <Windows.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include <SPIRV-Reflect/spirv_reflect.h>

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

PFN_vkSetDebugUtilsObjectNameEXT vk_SetDebugUtilsObjectNameEXT;
PFN_vkCreateRayTracingPipelinesKHR vk_CreateRayTracingPipelinesKHR;
PFN_vkGetAccelerationStructureBuildSizesKHR vk_GetAccelerationStructureBuildSizesKHR;
PFN_vkCreateAccelerationStructureKHR vk_CreateAccelerationStructureKHR;
PFN_vkDestroyAccelerationStructureKHR vk_DestroyAccelerationStructureKHR;

uint32_t get_memory_type_id(const VkPhysicalDeviceMemoryProperties2 mem_props, const VkMemoryRequirements2 mem_reqs, const uint32_t mem_prop_types);
void copy_buffer_to_buffer(const VkBuffer src_buffer, const VkBuffer dst_buffer, const VkBufferCopy2* regions, const uint32_t regions_count, const VkCommandBuffer cmd_buff, const VkQueue xfer_q);
void copy_buffer_to_image(const VkBuffer src_buffer, const VkImage dst_image, const VkImageLayout dst_image_layout, const VkBufferImageCopy2* regions, const uint32_t regions_count, const VkCommandBuffer cmd_buff, const VkQueue xfer_q);

static inline void VK_CHECK(const char* action, const VkResult result)
{
    if (result < VK_SUCCESS)
    {
        printf("VK ERR: %s %d\nExiting...\n", action, result);
        exit(result);
    }
}

static inline void SPV_CHECK(const char* action, const SpvReflectResult result)
{
    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        printf("SPV ERR: %s %d\nExiting...\n", action, result);
        exit(result);
    }
}

VkInstance vk_instance_create(void);
void vk_instance_destroy(VkInstance instance);


typedef struct surface_data
{
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR surf_caps;
    VkPresentModeKHR present_mode;
    VkSurfaceFormatKHR format;
    HINSTANCE h_instance;
    HWND h_wnd;
} surface_data;

surface_data vk_surface_create(const VkInstance instance, const HINSTANCE h_instance, const HWND h_wnd);
void vk_surface_destroy(VkSurfaceKHR surface, const VkInstance instance);

typedef struct phy_dev_data
{
    VkPhysicalDevice phy_dev;
    uint32_t q_count;
    uint32_t q_fly_idx;
    VkPhysicalDeviceProperties2 props;
    VkPhysicalDeviceMemoryProperties2 mem_props;
} phy_dev_data;

phy_dev_data vk_get_phy_dev_s(const VkInstance instance, surface_data * s);
phy_dev_data vk_get_phy_dev(const VkInstance instance);

VkDevice vk_device_create(const VkPhysicalDevice phy_dev, const uint32_t q_fly_idx, const uint32_t q_count);
void vk_device_destroy(VkDevice device);

typedef struct swapchain_data
{
    VkSwapchainKHR swapchain;
    VkCommandPool cmd_pool;

    VkImage* images;
    VkImageView* image_views;
    VkCommandBuffer* cmd_buffs;
    VkSemaphore* rndr_semaphores;
    VkFence* present_fences;
    uint32_t images_count;
} vk_swapchain_data;

vk_swapchain_data vk_swapchain_create(const VkDevice device, const surface_data* sd, const phy_dev_data* pd, const char* name);
void vk_swapchain_destroy(vk_swapchain_data sd, const VkDevice device);

typedef struct vk_cmd_pool_data
{
    VkCommandPool cmd_pool;
    VkCommandBuffer* cmd_buffs;
    uint32_t cmd_buffs_count;
} vk_cmd_pool_data;

vk_cmd_pool_data vk_command_pool_create(const VkDevice device, const uint32_t q_fly_idx, const uint32_t cmd_buffs_count, const char* name);
void vk_command_pool_destroy(vk_cmd_pool_data cd, const VkDevice device);

typedef struct vk_semaphore_data
{
    VkSemaphore semaphore;
    VkSemaphoreType type;
} vk_semaphore_data;

vk_semaphore_data vk_semaphore_create(const VkDevice device, const VkSemaphoreType semaphore_type, const char* name);
void vk_semaphore_destroy(VkSemaphore semaphore, const VkDevice device);

VkBuffer vk_buffer_create(const VkDevice device, const VkDeviceSize size, const VkBufferUsageFlags usage, const char* name);
void vk_buffer_destroy(VkBuffer buffer, const VkDevice device);

VkDeviceMemory vk_device_memory_allocate(const VkDevice device, const VkDeviceSize size, const uint32_t type_id, const VkMemoryAllocateFlagsInfo flags_info, const char* name);
void vk_device_memory_free(VkDeviceMemory memory, const VkDevice device);

VkImage vk_image_create(const VkDevice device, const VkExtent3D extent, const VkFormat format, const VkImageUsageFlags usage, const char* name);
void vk_image_destroy(VkImage image, const VkDevice device);

VkImageView vk_image_view_create(const VkDevice device, const VkImage image, const VkImageViewType view_type, const VkFormat format, const VkImageAspectFlags aspect_mask, const char* name);
void vk_image_view_destroy(VkImageView image_view, const VkDevice device);

typedef struct vk_pipeline_data
{
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    VkDescriptorSetLayout* dsls;
    uint32_t dsls_count;
} vk_pipeline_data;

vk_pipeline_data vk_raster_pipeline_create(const VkDevice device, const char* path, const VkFormat format, const char* name);
void vk_raster_pipeline_destroy(vk_pipeline_data d, const VkDevice device);

vk_pipeline_data vk_rt_pipeline_create(const VkDevice device, const char* path, const VkFormat format, const char* name);
void vk_rt_pipeline_destroy(vk_pipeline_data d, const VkDevice device);
