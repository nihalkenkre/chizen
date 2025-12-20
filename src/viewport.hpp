#pragma once

class VulkanInterface;
class Swapchain;
class FrameObjects;
class RasterizerPipelineData;
class ImGUIState;
class ViewportScene;
class ImageResource;
class Allocator;
class TransferHelpers;

class Viewport
{
public:
	Viewport() = delete;

	Viewport(const VulkanInterface* vulkan_interface, const std::string& current_path, const std::string& name);

	Viewport(const Viewport& other) = delete;
	Viewport& operator=(const Viewport& other) = delete;

	~Viewport() noexcept;

	void Render(const ViewportScene* scene, const Swapchain* swapchain, const VkExtent2D extent, ImGUIState* imgui_state);
	void RecreateDepthTexture(const VkExtent2D extent);

	const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const;

private:
	std::unique_ptr<FrameObjects> mFrameObjects;
	std::vector<VkSemaphore> mPresentWaitSemaphores;
	std::vector<VkSemaphore> mAcquireSignalSemaphores;
	std::unique_ptr<ImageResource> mDepthTexture;
	
	std::unique_ptr<RasterizerPipelineData> mPipelineData;
	std::vector<uint32_t> mQueueFamilyIndices;

	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;

	TransferHelpers* mTransferHelpers = nullptr;
	Allocator* mAllocator = nullptr;
	VkDevice mDevice = VK_NULL_HANDLE;
	VkQueue mQueue = VK_NULL_HANDLE;
	uint8_t mMaxFramesInFlight = 0;
};