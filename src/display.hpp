#pragma once

class VulkanInterface;
class FrameObjects;
class DisplayPipelineData;
class ImGUIState;
class DeviceBufferResource;
class ImageResource;
class TransferObjects;
class Swapchain;

class Display
{
public:
	Display() = delete;
	Display(const VulkanInterface* vulkan_interface, ImageResource* final_render_target, const std::string& current_path);

	Display(const Display& other) = delete;
	Display& operator=(const Display& other) = delete;

	~Display() noexcept;

	void Render(const float position_offset[], const float zoom_level, ImGUIState* imgui_state);
	void UpdateFinalRenderTarget(ImageResource* final_render_target);
	void UpdateSwapchain(Swapchain* swapchain);
	void UpdateExtent(VkExtent2D extent);

private:
	ImageResource* mFinalRenderTarget = nullptr;

	std::unique_ptr<FrameObjects> mFrameObjects = nullptr;
	std::vector<VkSemaphore> mPresentWaitSemaphores;
	std::vector<VkSemaphore> mAcquireSignalSemaphores;
	std::vector<VkDescriptorSet> mDescriptorSets;
	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;

	std::unique_ptr<DisplayPipelineData> mPipelineData = {};
	std::unique_ptr<DeviceBufferResource> mGeometryBuffer;

	TransferObjects* mTransferObjects = {};
	Swapchain* mSwapchain = {};
	VkExtent2D mExtent = {};
	VkQueue mQueue = VK_NULL_HANDLE;
	VkDevice mDevice = VK_NULL_HANDLE;

	uint8_t mMaxFramesInFlight = 0;
	uint8_t mFrameInFlight = 0;
};
