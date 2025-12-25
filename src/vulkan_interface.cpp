#include "vulkan_interface.hpp"
#include "utils.hpp"
#include "vulkan_objects.hpp"

VulkanInterface::VulkanInterface(SDL_Window* window)
{
	Uint32 vk_extensions_count = 0;
	const char* const* vk_extensions = SDL_Vulkan_GetInstanceExtensions(&vk_extensions_count);

	mInstance = std::make_unique<Instance>(vk_extensions, vk_extensions_count);
	mSurface = std::make_unique<Surface>(window, mInstance->GetInstance());
	mPhysicalDeviceData = std::make_unique<PhysicalDeviceData>(mInstance->GetPhysicalDeviceData(mSurface->GetSurfaceKHR()));
	mSurface->PopulateSurfaceData(mPhysicalDeviceData->PhysicalDevice);
	mDevice = std::make_unique<Device>(mPhysicalDeviceData.get());
	mSwapchain = std::make_unique<Swapchain>(mDevice->GetDevice(), mSurface.get(), mPhysicalDeviceData->GraphicsQueueFamilyIndex, "swapchain");
	mAllocator = std::make_unique<Allocator>(mInstance->GetInstance(), mPhysicalDeviceData->PhysicalDevice, mDevice->GetDevice());
	mTransferHelpers = std::make_unique<TransferHelpers>(mDevice->GetDevice(), mDevice->GetTransferQueue(), mPhysicalDeviceData->TransferQueueFamilyIndex, "transfer helpers");
	mComputeHelpers = std::make_unique<ComputeHelpers>(mDevice->GetDevice(), mDevice->GetComputeQueue(), mPhysicalDeviceData->ComputeQueueFamilyIndex, "compute helpers");

#ifdef _DEBUG
	Utils_SetObjectName(mDevice->GetDevice(), VK_OBJECT_TYPE_INSTANCE, reinterpret_cast<uint64_t>(mInstance->GetInstance()), "instance");
	Utils_SetObjectName(mDevice->GetDevice(), VK_OBJECT_TYPE_SURFACE_KHR, reinterpret_cast<uint64_t>(mSurface->GetSurfaceKHR()), "surface");
#endif // _DEBUG
}

Instance* VulkanInterface::GetInstance() const
{
	return mInstance.get();
}

PhysicalDeviceData* VulkanInterface::GetPhysicalDeviceData() const
{
	return mPhysicalDeviceData.get();
}

Surface* VulkanInterface::GetSurfaceKHR() const
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

TransferHelpers* VulkanInterface::GetTransferHelpers() const
{
	return mTransferHelpers.get();
}

ComputeHelpers* VulkanInterface::GetComputeHelpers() const
{
	return mComputeHelpers.get();
}

VkDevice VulkanInterface::GetVkDevice() const
{
	return mDevice->GetDevice();
}

VmaAllocator VulkanInterface::GetVmaAllocator() const
{
	return mAllocator->GetAllocator();
}

void VulkanInterface::RecreateViewportSwapchain()
{
	VK_CHECK("queue wait idle", vkQueueWaitIdle(mDevice->GetGraphicsQueue()));

	mSwapchain.reset();
	mSurface->PopulateSurfaceData(mPhysicalDeviceData->PhysicalDevice);
	mSwapchain = std::make_unique<Swapchain>(mDevice->GetDevice(), mSurface.get(), mPhysicalDeviceData->GraphicsQueueFamilyIndex, "swapchain");
}
