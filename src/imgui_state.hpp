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

	void Render(const VkCommandBuffer cmd_buff);

	bool& GetShouldStartRaytracing();
	int& GetMaxSamples();
	int* GetRenderTargetExtent();

	uint32_t GetFileOpenEventType() const;
	uint32_t GetStartRaytraceEventType() const;
	uint32_t GetStopRaytraceEventType() const;

private:
	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
	VkDevice mDevice = VK_NULL_HANDLE;

	bool mShouldStartRaytracing = false;
	int mMaxSamples = 1024;
	int mRenderTargetExtent[2] = { 1280, 720 };
	std::string file_path = {};

	SDL_Event mFileOpenEvent = {};
	SDL_Event mStartRayTraceEvent = {};
	SDL_Event mStopRaytraceEvent = {};
};