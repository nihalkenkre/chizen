#include "vulkan_interface.hpp"
#include "utils.hpp"

//VulkanInterface::VulkanInterface(SDL_Window* window)
//{
//	std::println("VulkanInteface ctor");
//
//	Uint32 vk_extensions_count = 0;
//	const char* const* vk_extensions = SDL_Vulkan_GetInstanceExtensions(&vk_extensions_count);
//
//	instance = vk_instance(vk_extensions, vk_extensions_count);
//	surface = vk_surface(window, instance.instance, nullptr);
//}
//
//VulkanInterface::VulkanInterface(VulkanInterface&& other) noexcept
//{
//	std::println("vulkan interface move ctor");
//	*this = std::move(other);
//}
//
//VulkanInterface& VulkanInterface::operator=(VulkanInterface&& other) noexcept
//{
//	std::println("vulkan interface move ass");
//
//	instance = std::move(other.instance);
//	surface = std::move(other.surface);
//
//	return *this;
//}
//
//VulkanInterface::~VulkanInterface() noexcept
//{
//}

VulkanInterface VulkanInterface_Create(SDL_Window* window)
{
	Uint32 vk_extensions_count = 0;
	const char* const* vk_extensions = SDL_Vulkan_GetInstanceExtensions(&vk_extensions_count);
	
	VulkanInterface vi = {};

	vi.Instance = VkInstance_Create(vk_extensions, vk_extensions_count);
	SDL_Vulkan_CreateSurface(window, vi.Instance, nullptr, &vi.SurfaceData.Surface);
	vi.PhysicalDeviceData = VkInstance_GetPhysicalDeviceData(vi.Instance, vi.SurfaceData.Surface);
	vi.SurfaceData = VkPhysicalDevice_GetSurfaceData(vi.PhysicalDeviceData.PhysicalDevice, vi.SurfaceData.Surface);
	vi.DeviceData = DeviceData_Create(vi.PhysicalDeviceData);
	vi.SwapchainData = SwapchainData_Create(vi.DeviceData.Device, vi.SurfaceData, vi.PhysicalDeviceData.GraphicsQueueFamilyIndex, "swapchain");
	vi.Allocator = Allocator_Create(vi.Instance, vi.PhysicalDeviceData.PhysicalDevice, vi.DeviceData.Device);
	vi.TransferObjects = TransferObjects_Create(vi.DeviceData.Device, vi.DeviceData.TransferQueue, vi.PhysicalDeviceData.TransferQueueFamilyIndex);
	vi.FinalRenderTarget = ImageResource_Create(vi.DeviceData.Device, 
		{ 1280, 720, 1 }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		vi.Allocator, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 
		{ vi.PhysicalDeviceData.GraphicsQueueFamilyIndex, vi.PhysicalDeviceData.ComputeQueueFamilyIndex, vi.PhysicalDeviceData.TransferQueueFamilyIndex }, 
		"final render target");

	Utils_InitializeImages({ vi.FinalRenderTarget.Image }, vi.TransferObjects.CommandBuffer, vi.TransferObjects.Queue);

	return vi;
}

void VulkanInterface_Destroy(VulkanInterface* vi)
{
	ImageResource_Destroy(vi->FinalRenderTarget);
	TransferObjects_Destroy(vi->TransferObjects);
	Allocator_Destroy(vi->Allocator);
	SwapchainData_Destroy(vi->SwapchainData);
	DeviceData_Destroy(vi->DeviceData);
	SDL_Vulkan_DestroySurface(vi->Instance, vi->SurfaceData.Surface, nullptr);
	VkInstance_Destroy(vi->Instance);
}

void VulkanInterface_RecreateFinalRenderTarget(VulkanInterface* vi, const VkExtent2D& extent)
{
	ImageResource_Destroy(vi->FinalRenderTarget);
	vi->FinalRenderTarget = ImageResource_Create(vi->DeviceData.Device,
		{extent.width, extent.height, 1}, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		vi->Allocator, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 
		{ vi->PhysicalDeviceData.GraphicsQueueFamilyIndex, vi->PhysicalDeviceData.ComputeQueueFamilyIndex, vi->PhysicalDeviceData.TransferQueueFamilyIndex }, 
		"final render target");

	Utils_InitializeImages({ vi->FinalRenderTarget.Image }, vi->TransferObjects.CommandBuffer, vi->TransferObjects.Queue);
}
