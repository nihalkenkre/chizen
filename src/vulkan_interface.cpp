#include "vulkan_interface.hpp"
#include "utils.hpp"
#include "vulkan_objects.hpp"

VulkanInterface::VulkanInterface(SDL_Window* window)
{
	Uint32 vk_extensions_count = 0;
	const char* const* vk_extensions = SDL_Vulkan_GetInstanceExtensions(&vk_extensions_count);

	mInstance = std::make_unique<Instance>(vk_extensions, vk_extensions_count);
	mSurface = std::make_unique<Surface>(window, mInstance->GetInstance());
	mPhysicalDeviceData = std::make_unique<PhysicalDeviceData>(mInstance->GetPhysicalDeviceData(mSurface->GetSurface()));
	mSurface->PopulateSurfaceData(mPhysicalDeviceData->PhysicalDevice);
	mDevice = std::make_unique<Device>(mPhysicalDeviceData.get());
	mSwapchain = std::make_unique<Swapchain>(mDevice->GetDevice(), mSurface.get(), mPhysicalDeviceData->GraphicsQueueFamilyIndex, "swapchain");
	mAllocator = std::make_unique<Allocator>(mInstance->GetInstance(), mPhysicalDeviceData->PhysicalDevice, mDevice->GetDevice());
	mTransferObjects = std::make_unique<TransferObjects>(mDevice->GetDevice(), mDevice->GetTransferQueue(), mPhysicalDeviceData->TransferQueueFamilyIndex);
}

Instance* VulkanInterface::GetInstance() const
{
	return mInstance.get();
}

PhysicalDeviceData* VulkanInterface::GetPhysicalDeviceData() const
{
	return mPhysicalDeviceData.get();
}

Surface* VulkanInterface::GetSurface() const
{
	return mSurface.get();
}

Device* VulkanInterface::GetDevice() const
{
	return mDevice.get();
}

Swapchain* VulkanInterface::GetSwapchain() const
{
	return mSwapchain.get();
}

Allocator* VulkanInterface::GetAllocator() const
{
	return mAllocator.get();
}

TransferObjects* VulkanInterface::GetTransferObjects() const
{
	return mTransferObjects.get();
}
