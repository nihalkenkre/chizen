#pragma once

class VulkanInterface;
class FrameObjects;
class DisplayPipelineData;
class ImGUIState;
class DeviceBufferResource;
class ImageResource;
class TransferHelpers;
class Swapchain;

class Display
{
public:
	Display() = delete;
	Display(const VulkanInterface* vulkan_interface, const std::string& current_path);

	Display(const Display& other) = delete;
	Display& operator=(const Display& other) = delete;

	~Display() noexcept;

	void Render(const ImageResource* final_render_target, const Swapchain* swapchain, const VkExtent2D extent, const float position_offset[], const float zoom_level, ImGUIState* imgui_state);

private:

	std::unique_ptr<FrameObjects> mFrameObjects = nullptr;
	std::vector<VkSemaphore> mPresentWaitSemaphores;
	std::vector<VkSemaphore> mAcquireSignalSemaphores;
	std::vector<VkDescriptorSet> mDescriptorSets;
	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;

	std::unique_ptr<DisplayPipelineData> mPipelineData = {};
	std::unique_ptr<DeviceBufferResource> mGeometryBuffer;

	TransferHelpers* mTransferHelpers = {};
	VkQueue mQueue = VK_NULL_HANDLE;
	VkDevice mDevice = VK_NULL_HANDLE;

	uint8_t mMaxFramesInFlight = 0;
	uint8_t mFrameInFlight = 0;
};
