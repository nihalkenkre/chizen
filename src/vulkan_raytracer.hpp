#pragma once

class VulkanInterface;
class FrameObjects;
class VulkanRaytracerScenePipelineData;
class ImageResource;
class HostBufferResource;
class DeviceBufferResource;
class VulkanRaytracerScene;
class TransferHelpers;

class VulkanRaytracer
{
public:
	VulkanRaytracer() = delete;
	VulkanRaytracer(const VulkanInterface* const vulkan_interface, const VkExtent3D& extent);

	VulkanRaytracer(const VulkanRaytracer& other) = delete;
	VulkanRaytracer& operator=(const VulkanRaytracer& other) = delete;

	~VulkanRaytracer() noexcept;

	void RecreateRenderResources(const VkExtent3D& extent);
	void Start(const VulkanRaytracerScene* scene, const ImageResource* final_render_target, const VkExtent3D& extemt, const uint32_t max_samples, const uint32_t cam_index, const bool is_cpu_shading);
	void Stop();

	const FrameObjects* GetFrameObjects() const;
	const ImageResource* GetAccumRenderTarget() const;

private:
	void InitializeResources(const VkExtent3D& extent);

	std::unique_ptr<ImageResource> mAccumRenderTarget = nullptr;
	std::unique_ptr<DeviceBufferResource> mRandomStates = nullptr;

	std::unique_ptr<FrameObjects> mFrameObjects = nullptr;
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR mRayTracingProperties = {};

	TransferHelpers* mTransferHelpers = nullptr;
	VkQueue mComputeQueue = VK_NULL_HANDLE;
	std::vector<uint32_t> mQueueFamilyIndices;
	VmaAllocator mAllocator = nullptr;
	VkDevice mDevice = VK_NULL_HANDLE;

	uint8_t mMaxFramesInFlight = 0;
	bool mStopRendering = false;
};