#pragma once

#include "vulkan_objects.hpp"
#include "resources.hpp"

class VulkanInterface
{
public:
	VulkanInterface() = delete;
	VulkanInterface(SDL_Window* window);

	VulkanInterface(const VulkanInterface& other) = delete;
	VulkanInterface& operator=(const VulkanInterface& other) = delete;

	~VulkanInterface() noexcept;

	void RecreateFinalRenderTarget(const VkExtent3D& extent);

	Instance* GetInstance() const;
	PhysicalDeviceData* GetPhysicalDeviceData() const;
	Surface* GetSurface() const;
	Device* GetDevice() const;
	Swapchain* GetSwapchain() const;
	Allocator* GetAllocator() const;
	TransferObjects* GetTransferObjects() const;
	ImageResource* GetFinalRenderTarget() const;

private:
	std::unique_ptr<Instance> mInstance;
	std::unique_ptr<PhysicalDeviceData> mPhysicalDeviceData;
	std::unique_ptr<Surface> mSurface;
	std::unique_ptr<Device> mDevice;
	std::unique_ptr<Swapchain> mSwapchain;
	std::unique_ptr<Allocator> mAllocator;
	std::unique_ptr<TransferObjects> mTransferObjects;
	std::unique_ptr<ImageResource> mFinalRenderTarget;
};
