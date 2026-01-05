#pragma once

#include "scene.hpp"

class ComputeHelpers;
class DeviceBufferResource;
class BLAccelerationStructure;
class TLAccelerationStructure;
class VulkanRaytracerScenePipelineData;
class ImageResource;
class ComputeHelpers;
class Pool;
class Scene;
class VulkanScene;

class VulkanRaytracerScene
{
public:
	VulkanRaytracerScene() = delete;
	VulkanRaytracerScene(const Scene& scene, const VulkanScene* vulkan_scene, const VulkanInterface* vulkan_interface);
	
	VulkanRaytracerScene(const VulkanRaytracerScene& other) = delete;
	VulkanRaytracerScene& operator=(const VulkanRaytracerScene& other) = delete;

	~VulkanRaytracerScene() noexcept;

	void Render(const VkCommandBuffer cmd_buff, const DeviceBufferResource* rand_states, const ImageResource* accum_target, const ImageResource* final_render_target, const uint32_t current_sample, const uint32_t width, const uint32_t height, const uint32_t cam_index) const;

	VkAccelerationStructureKHR GetTLAS() const;
	VulkanRaytracerScenePipelineData* GetPipelineData() const;

private:
	std::unique_ptr<Pool> mScratchBufferPool = nullptr;
	std::unique_ptr<Pool> mSBTBufferPool = nullptr;

	std::unique_ptr<TLAccelerationStructure> mTLAS;
	std::vector<std::unique_ptr<BLAccelerationStructure>> mBLASes;
	std::unique_ptr<VulkanRaytracerScenePipelineData> mPipelineData = nullptr;
	
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR mRaytracingProperties = {};
	std::unique_ptr<DeviceBufferResource> mRGSbt = nullptr;
	std::unique_ptr<DeviceBufferResource> mMSSbt = nullptr;
	std::unique_ptr<DeviceBufferResource> mCHSbt = nullptr;

	VkDeviceSize mCHSbtRecordAlignedSize = 0;
	VkDeviceSize mCHSbtAlignedSize = 0;

	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet mCameraDescSet = VK_NULL_HANDLE;
	VkDescriptorSet mSceneDescSet = VK_NULL_HANDLE;
	VkDescriptorBufferInfo mCameraDescBuffer = {};

	const VulkanScene* mScene = nullptr;
	VkDevice mDevice = VK_NULL_HANDLE;
};
