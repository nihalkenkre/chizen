#pragma once

class VulkanInterface;
class FrameObjects;
class RaytracerPipelineData;
class ImageResource;
class HostBufferResource;
class DeviceBufferResource;
class VulkanRaytracerScene;
class TransferHelpers;

class VulkanRaytracer
{
public:
	VulkanRaytracer() = delete;
	VulkanRaytracer(const VulkanInterface* const vulkan_interface, const VkExtent3D& extent, const std::string& current_path, const std::string& name);

	VulkanRaytracer(const VulkanRaytracer& other) = delete;
	VulkanRaytracer& operator=(const VulkanRaytracer& other) = delete;

	~VulkanRaytracer() noexcept;

	void RecreateRenderResources(const VkExtent2D& extent);
	void Start(const VulkanRaytracerScene* scene, const ImageResource* final_render_target, const uint32_t max_samples);
	void Stop();

	FrameObjects* GetFrameObjects() const;

private:
	void InitializeResources();

	std::unique_ptr<ImageResource> mAccumRenderTarget = nullptr;
	std::unique_ptr<DeviceBufferResource> mRandomStates = nullptr;
	std::unique_ptr<HostBufferResource> mRaygenSBT = nullptr;
	std::unique_ptr<HostBufferResource> mMissSBT = nullptr;
	std::unique_ptr<HostBufferResource> mCHSBT = nullptr;

	std::unique_ptr<FrameObjects> mFrameObjects = nullptr;
	std::unique_ptr<HostBufferResource> mUniformBuffer = nullptr;
	std::vector<VkDescriptorSet> mDescriptorSets;
	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR mRayTracingProperties = {};

	std::unique_ptr<RaytracerPipelineData> mPipelineData = nullptr;

	TransferHelpers* mTransferHelpers = nullptr;
	VkExtent3D mExtent = {};
	VkQueue mComputeQueue = VK_NULL_HANDLE;
	std::vector<uint32_t> mQueueFamilyIndices;
	VmaAllocator mAllocator = nullptr;
	VkDevice mDevice = VK_NULL_HANDLE;

	uint8_t mMaxFramesInFlight = 0;
	uint8_t mFrameInFlight = 0;
	bool mStopRendering = false;
};