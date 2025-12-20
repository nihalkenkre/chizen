#pragma once

#include "scene.hpp"

class ComputeHelpers;
class DeviceBufferResource;
class BLAccelerationStructure;
class TLAccelerationStructure;

class VulkanRaytracerScene
{
public:
	VulkanRaytracerScene() = delete;

	VulkanRaytracerScene(const Scene& scene, const VkDevice device, const VmaAllocator allocator, const size_t uniform_buffer_alignment, ComputeHelpers* compute_helpers);
	
	VulkanRaytracerScene(const VulkanRaytracerScene& other) = delete;
	VulkanRaytracerScene& operator=(const VulkanRaytracerScene& other) = delete;

	~VulkanRaytracerScene() noexcept;

	VkAccelerationStructureKHR GetTLAS() const;


private:
	std::unique_ptr<TLAccelerationStructure> mTLAS;
	std::vector<std::unique_ptr<BLAccelerationStructure>> mBLASes;

	VkDescriptorPool mDescPool = VK_NULL_HANDLE;
	VkDevice mDevice = VK_NULL_HANDLE;
};
