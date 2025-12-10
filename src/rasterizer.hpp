#pragma once

class VulkanInterface;
class Swapchain;
class FrameObjects;
class RasterizerPipelineData;
class ImGUIState;
class Scene;
class ImageResource;
class Allocator;

class Rasterizer
{
public:
	Rasterizer() = delete;

	Rasterizer(const VulkanInterface* vulkan_interface, const std::string& current_path, const std::string& name);

	Rasterizer(const Rasterizer& other) = delete;
	Rasterizer& operator=(const Rasterizer& other) = delete;

	~Rasterizer() noexcept;

	void Render(const Scene* scene, ImGUIState* imgui_state);
	void UpdateSwapchain(Swapchain* swapchain);
	void UpdateExtent(const VkExtent2D& extent);
	void RecreateDepthTexture();
	void InitializeResources(const VkCommandBuffer cmd_buff, const VkQueue queue);

	const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const;

private:
	std::unique_ptr<FrameObjects> mFrameObjects;
	std::vector<VkSemaphore> mPresentWaitSemaphores;
	std::vector<VkSemaphore> mAcquireSignalSemaphores;
	std::unique_ptr<ImageResource> mDepthTexture;
	
	std::unique_ptr<RasterizerPipelineData> mPipelineData;
	std::vector<uint32_t> mQueueFamilyIndices;

	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;

	Allocator* mAllocator = nullptr;
	Swapchain* mSwapchain = nullptr;
	VkDevice mDevice = VK_NULL_HANDLE;
	VkExtent2D mExtent = { 1280, 720 };
	VkQueue mQueue = VK_NULL_HANDLE;
	uint8_t mMaxFramesInFlight = 0;
};