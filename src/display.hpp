#pragma once

#include "vulkan_interface.hpp"
#include "utils.hpp"

class FrameObjects;
class DisplayPipelineData;

class Display
{
public:
	Display() = delete;
	Display(const VulkanInterface* vulkan_interface, const std::string& current_path);

	Display(const Display& other) = delete;
	Display& operator=(const Display& other) = delete;

	~Display() noexcept;

	void Render(const float position_offset[], const float zoom_level, ImGUIState* imgui_state);
	void UpdateFinalRenderTarget(ImageResource* FinalRenderTarget);

private:
	ImageResource* mFinalRenderTarget = nullptr;

	std::unique_ptr<FrameObjects> mFrameObjects = nullptr;
	std::vector<VkSemaphore> mPresentWaitSemaphores;
	std::vector<VkSemaphore> mAcquireSignalSemaphores;
	std::vector<VkDescriptorSet> mDescriptorSets;
	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;

	std::unique_ptr<DisplayPipelineData> mPipelineData = {};
	std::unique_ptr<BufferResource> mGeometryBuffer;

	Swapchain* mSwapchain = {};
	VkExtent2D mExtent = {};
	VkQueue mQueue = VK_NULL_HANDLE;
	VkDevice mDevice = VK_NULL_HANDLE;

	uint8_t mMaxFramesInFlight = 0;
	uint8_t mFrameInFlight = 0;
};
