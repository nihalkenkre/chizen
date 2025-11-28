#pragma once

class VulkanInterface;

class ImGUIState
{
public:
	ImGUIState() = delete;
	ImGUIState(const VulkanInterface* vulkan_interface);
	
	ImGUIState(const ImGUIState& other) = delete;
	ImGUIState& operator=(const ImGUIState& other) = delete;

	~ImGUIState() noexcept;

	bool& GetStartRaytracing();
	int& GetMaxSamples();
	int* GetRenderTargetExtent();

private:
	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
	VkDevice mDevice = VK_NULL_HANDLE;

	bool mStartRaytracing = false;
	int mMaxSamples = 1024;
	int mRenderTargetExtent[2] = { 1280, 720 };
};