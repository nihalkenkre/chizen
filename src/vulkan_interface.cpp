#include "vulkan_interface.hpp"
#include "utils.hpp"

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

	VkExtent3D extent = { 1280, 720, 1 };
	std::vector<uint32_t> queue_family_indices{ mPhysicalDeviceData->GraphicsQueueFamilyIndex, mPhysicalDeviceData->ComputeQueueFamilyIndex, mPhysicalDeviceData->TransferQueueFamilyIndex };
	mFinalRenderTarget = std::make_unique<ImageResource>(mDevice->GetDevice(),
		extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		mAllocator->GetAllocator(), 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		queue_family_indices,
		"final render target");

	Utils_InitializeImages({ mFinalRenderTarget->GetImage() }, mTransferObjects->GetCommandBuffer(), mTransferObjects->GetQueue());
}

VulkanInterface::~VulkanInterface()
{
	//ImageResource_Destroy(mFinalRenderTarget);
	//mFinalRenderTarget.reset();
}

void VulkanInterface::RecreateFinalRenderTarget(const VkExtent3D& extent)
{
	//mFinalRenderTarget.reset();
	std::vector<uint32_t> queue_family_indices{ mPhysicalDeviceData->GraphicsQueueFamilyIndex, mPhysicalDeviceData->ComputeQueueFamilyIndex, mPhysicalDeviceData->TransferQueueFamilyIndex };
	mFinalRenderTarget = std::make_unique<ImageResource>(mDevice->GetDevice(),
		extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		mAllocator->GetAllocator(), 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		queue_family_indices,
		"final render target");

	Utils_InitializeImages({ mFinalRenderTarget->GetImage() }, mTransferObjects->GetCommandBuffer(), mTransferObjects->GetQueue());
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

ImageResource* VulkanInterface::GetFinalRenderTarget() const
{
	return mFinalRenderTarget.get();
}
