#pragma once

#include "vulkan_objects.hpp"
#include "resources.hpp"

//class VulkanInterface
//{
//public:
//   VulkanInterface() { std::println("VulkanInterface ctor def"); }
//   VulkanInterface(SDL_Window* window);
//
//   VulkanInterface(const VulkanInterface& other) = delete;
//   VulkanInterface& operator=(const VulkanInterface& other) = delete;
//
//   VulkanInterface(VulkanInterface&& other) noexcept;
//   VulkanInterface& operator=(VulkanInterface&& other) noexcept;
//
//   ~VulkanInterface() noexcept;
//
//private:
//   vk_instance instance;
//   vk_surface surface;
//   //VkSurfaceKHR surface;
//};

struct VulkanInterface
{
	VkInstance Instance;
	PhysicalDeviceData PhysicalDeviceData;
	SurfaceData SurfaceData;
	DeviceData DeviceData;
	SwapchainData SwapchainData;
	VmaAllocator Allocator;
	ImageResource FinalRenderTarget;
	TransferObjects TransferObjects;
};

VulkanInterface VulkanInterface_Create(SDL_Window* window);
void VulkanInterface_Destroy(VulkanInterface* vi);
void VulkanInterface_RecreateFinalRenderTarget(VulkanInterface* vulkan_interface, const VkExtent2D& extent);