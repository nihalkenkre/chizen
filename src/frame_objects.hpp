#pragma once

class FrameObjects
{
public:
	FrameObjects() = delete;
	FrameObjects(const VkDevice device, const uint32_t queue_family_index, const uint8_t& max_frames_in_flight);

	FrameObjects(const FrameObjects& other) = delete;
	FrameObjects& operator=(const FrameObjects& other) = delete;

	~FrameObjects() noexcept;

	uint8_t GetFrameInFlight() const;
	VkCommandBuffer GetCommandBuffer() const;
	VkSemaphore GetSemaphore() const;
	uint64_t& GetFrameSemValue();
	void NextFrame();

private:
	std::vector<VkSemaphore> mFrameSemaphores;
	std::vector<uint64_t> mFrameSemaphoreValues;
	std::vector<VkCommandBuffer> mCommandBuffers;
	VkCommandPool mCommandPool = VK_NULL_HANDLE;

	uint8_t mMaxFramesInFlight = 0;
	uint8_t mFrameInFlight = 0;

	VkDevice mDevice = VK_NULL_HANDLE;
};