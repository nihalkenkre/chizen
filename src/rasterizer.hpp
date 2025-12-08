#pragma once

class VulkanInterface;
class Swapchain;
class FrameObjects;

class Rasterizer
{
public:
	Rasterizer() = delete;

	Rasterizer(const VulkanInterface* vulkan_interface, const std::string& name);

	Rasterizer(const Rasterizer& other) = delete;
	Rasterizer& operator=(const Rasterizer& other) = delete;

	~Rasterizer() noexcept;

	void Render();
	void UpdateSwapchain(Swapchain* swapchain);

private:

	std::unique_ptr<FrameObjects> mFrameObjects;
	std::vector<VkSemaphore> mPresentWaitSemaphores;
	std::vector<VkSemaphore> mAcquireSignalSemaphores;
	std::vector<VkDescriptorSet> mDescriptorSets;

	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;

	Swapchain* mSwapchain = nullptr;
	VkDevice mDevice = VK_NULL_HANDLE;
	VkExtent2D mExtent = { 1280, 720 };
	VkQueue mQueue = VK_NULL_HANDLE;
	uint8_t mMaxFramesInFlight = 0;
};