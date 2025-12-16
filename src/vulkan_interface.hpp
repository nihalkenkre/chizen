#pragma once

class Instance;
struct PhysicalDeviceData;
class Surface;
class Device;
class Swapchain;
class Allocator;
class TransferObjects;
class ComputeHelpers;

class VulkanInterface
{
public:
	VulkanInterface() = delete;
	VulkanInterface(SDL_Window* window);

	VulkanInterface(const VulkanInterface& other) = delete;
	VulkanInterface& operator=(const VulkanInterface& other) = delete;

	Instance* GetInstance() const;
	PhysicalDeviceData* GetPhysicalDeviceData() const;
	Surface* GetSurfaceKHR() const;
	Device* GetDevice() const;
	Swapchain* GetSwapchain() const;
	Allocator* GetAllocator() const;
	TransferObjects* GetTransferObjects() const;
	ComputeHelpers* GetComputeHelpers() const;

	VkDevice GetVkDevice() const;
	VmaAllocator GetVmaAllocator() const;

	void RecreateRasterSwapchain();

private:
	std::unique_ptr<Instance> mInstance;
	std::unique_ptr<PhysicalDeviceData> mPhysicalDeviceData;
	std::unique_ptr<Surface> mSurface;
	std::unique_ptr<Device> mDevice;
	std::unique_ptr<Swapchain> mSwapchain;
	std::unique_ptr<Allocator> mAllocator;
	std::unique_ptr<TransferObjects> mTransferObjects;
	std::unique_ptr<ComputeHelpers> mComputeHelpers;
};
