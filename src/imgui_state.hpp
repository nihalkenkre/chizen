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

	void ProcessEvent(SDL_Event* event);
	void SetCameraNames(const std::vector<const char*>& names);
	void Render(const VkCommandBuffer cmd_buff);

	bool& GetShouldStartRaytracing();
	int& GetMaxSamples();
	int* GetRenderTargetExtent();
	const int GetSelectedCameraIndex() const;

private:
	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
	VkDevice mDevice = VK_NULL_HANDLE;

	 std::vector<const char*> mCameraNames = {};
	int mSelectedCameraIndex = 0;
	bool mShouldStartRaytracing = false;
	int mMaxSamples = 1024;
	int mRenderTargetExtent[2] = { 1280, 720 };
	std::string file_path = {};
	bool mRaytracingStarted = false;
};