#pragma once

class VulkanInterface;
class FrameObjects;
class RaytracePipelineData;
class ImageResource;
class BufferResource;

class Raytrace
{
public:
	Raytrace() = delete;
	Raytrace(const VulkanInterface* const vulkan_interface, ImageResource* final_render_target, const VkExtent3D& extent, const std::string& current_path, const std::string& name);

	Raytrace(const Raytrace& other) = delete;
	Raytrace& operator=(const Raytrace& other) = delete;

	~Raytrace() noexcept;

	void RecreateRenderResources(const VkExtent2D& extent);
	void Render(bool * is_raytracing, const uint32_t max_samples);
	void UpdateFinalRenderTarget( ImageResource* FinalRenderTarget);
	void StopRendering();

private:
	void InitializeResources();

	ImageResource* mFinalRenderTarget = {};
	std::unique_ptr<ImageResource> mAccumRenderTarget = nullptr;
	std::unique_ptr<BufferResource> mRandomStates = nullptr;
	std::unique_ptr<BufferResource> mRaygenSBT = nullptr;
	std::unique_ptr<BufferResource> mMissSBT = nullptr;
	std::unique_ptr<BufferResource> mCHSBT = nullptr;

	std::unique_ptr<FrameObjects> mFrameObjects = nullptr;
	std::unique_ptr<BufferResource> mUniformBuffer = nullptr;
	std::vector<VkDescriptorSet> mDescriptorSets;
	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR mRayTracingProperties = {};

	std::unique_ptr<RaytracePipelineData> mPipelineData = nullptr;

	VkExtent3D mExtent = {};
	VkQueue mComputeQueue = VK_NULL_HANDLE;
	VkQueue mTransferQueue = VK_NULL_HANDLE;
	VkCommandBuffer mTransferCommandBuffer = VK_NULL_HANDLE;
	std::vector<uint32_t> mQueueFamilyIndices;
	VmaAllocator mAllocator = nullptr;
	VkDevice mDevice = VK_NULL_HANDLE;

	uint8_t mMaxFramesInFlight = 0;
	uint8_t mFrameInFlight = 0;
	bool mStopRendering = false;
};